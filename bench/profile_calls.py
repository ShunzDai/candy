#!/usr/bin/env python3
"""Compile-only versus precompiled execution profiling for the stack VM.

Build explicitly against a preserved baseline or candidate core archive:
  python3 bench/profile_calls.py --core build/baseline-core.a \
      --binary build/profile_calls-baseline --build-only
  python3 bench/profile_calls.py --core build/src/core/libcandy_core.a \
      --binary build/profile_calls-candidate --build-only

Tiny correctness smoke (not a benchmark):
  python3 bench/profile_calls.py --binary build/profile_calls-candidate \
      --calls 10 --reps 2 --warmup 1

Only run the larger diagnosis when requested:
  python3 bench/profile_calls.py --binary build/profile_calls-baseline \
      --calls 200000 --reps 20 --warmup 2 --samples 5

The harness compiles the source once per process, then repeatedly executes the
same rooted closure. Each execution defines id, makes CALLS discarded calls,
and returns id(1). Untimed calls validate integer 1 before/after measurement.
Runtime GC polling remains enabled; explicit full collections are separate.
All Candy timings are CLOCK_MONOTONIC wall nanoseconds, not process startup or
CPU time. No Lua comparison is implied. --mode loop measures additional loop
and global-variable overhead, unlike the default unrolled call sequence.
Matching headers/configuration must accompany each archive; --include-src and
--config-dir support archives built in another checkout/build directory.
"""
from __future__ import annotations

import argparse
import hashlib
import json
from pathlib import Path
import shlex
import statistics
import subprocess
import sys

ROOT = Path(__file__).resolve().parents[1]


def nonnegative(text: str) -> int:
    number = int(text)
    if number < 0:
        raise argparse.ArgumentTypeError("must be nonnegative")
    return number


def positive(text: str) -> int:
    number = nonnegative(text)
    if number == 0:
        raise argparse.ArgumentTypeError("must be positive")
    return number


def parser() -> argparse.ArgumentParser:
    p = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    p.add_argument("--binary", type=Path, default=ROOT / "build/profile_calls",
                   help="harness output/existing binary (default: build/profile_calls)")
    p.add_argument("--core", type=Path, help="compile harness against this existing core .a; never rebuild core")
    p.add_argument("--include-src", type=Path, default=ROOT / "src", help="matching source include directory")
    p.add_argument("--config-dir", type=Path, default=ROOT / "build", help="directory containing matching candy_config.h")
    p.add_argument("--cc", default="/usr/bin/clang", help="compiler command, shell-style arguments supported")
    p.add_argument("--cflags", default="-std=c99 -O3 -DNDEBUG -flto -Wall -Wextra -Werror",
                   help="harness compile flags; does not change archive optimization")
    p.add_argument("--ldflags", default="-lm", help="additional linker flags")
    p.add_argument("--build-only", action="store_true", help="build harness without running any measurements")
    p.add_argument("--calls", type=nonnegative, default=200_000, help="discarded id calls per execution")
    p.add_argument("--reps", type=positive, default=20, help="timed executions per compiled closure")
    p.add_argument("--warmup", type=nonnegative, default=2, help="untimed executions, besides validation")
    p.add_argument("--samples", type=positive, default=1, help="independent processes/compilations")
    p.add_argument("--mode", choices=("unrolled", "loop"), default="unrolled")
    p.add_argument("--label", default="", help="baseline/candidate label copied to output")
    return p


def build(args: argparse.Namespace) -> list[str] | None:
    if args.core is None:
        if args.build_only:
            raise ValueError("--build-only requires --core")
        return None
    core = args.core.resolve(strict=True)
    source = ROOT / "bench/profile_calls.c"
    config = args.config_dir.resolve(strict=True)
    if not (config / "candy_config.h").is_file():
        raise ValueError(f"no candy_config.h in {config}")
    args.binary.parent.mkdir(parents=True, exist_ok=True)
    command = [*shlex.split(args.cc), *shlex.split(args.cflags),
               "-I", str(args.include_src.resolve(strict=True)), "-I", str(config),
               str(source), str(core), *shlex.split(args.ldflags), "-o", str(args.binary)]
    print("+ " + shlex.join(command), file=sys.stderr)
    subprocess.run(command, check=True)
    return command


def run(args: argparse.Namespace) -> dict:
    command = build(args)
    binary = args.binary.resolve(strict=True)
    metadata = {"label": args.label, "binary": str(binary),
                "binary_sha256": hashlib.sha256(binary.read_bytes()).hexdigest(),
                "build_command": command}
    if args.core is not None:
        metadata["core"] = str(args.core.resolve())
        metadata["core_sha256"] = hashlib.sha256(args.core.read_bytes()).hexdigest()
    if args.build_only:
        return {"schema": 1, "build_only": True, **metadata}
    results = []
    for _ in range(args.samples):
        completed = subprocess.run([str(binary), str(args.calls), str(args.reps),
                                    str(args.warmup), args.mode],
                                   check=True, text=True, capture_output=True)
        if completed.stderr:
            print(completed.stderr, end="", file=sys.stderr)
        item = json.loads(completed.stdout)
        if (item.get("result") != 1 or item.get("clock") != "monotonic_wall" or
                item.get("calls") != args.calls or item.get("mode") != args.mode or
                item.get("reps") != args.reps or len(item.get("execute_ns", [])) != args.reps):
            raise ValueError("unexpected/invalid harness result")
        results.append(item)
    times = [time for result in results for time in result["execute_ns"]]
    return {"schema": 1, **metadata, "engine": "candy", "clock": "monotonic_wall",
            "unit": "ns", "samples": results,
            "summary": {"compile_median_ns": statistics.median(r["compile_ns"] for r in results),
                        "execute_median_ns": statistics.median(times),
                        "execute_min_ns": min(times),
                        "execute_median_ns_per_call": statistics.median(times) / (args.calls + 1),
                        "timed_executions": len(times)},
            "notes": ["Source generation, state setup/teardown and validation are untimed.",
                      "Compilation includes parse/codegen/allocation; explicit full GC is reported separately.",
                      "Execution includes closure/global setup and normal runtime GC polling, not explicit full GC.",
                      "Compare only matching modes/inputs/build flags; no CPU-time Lua comparison."]}


def main() -> int:
    args = parser().parse_args()
    try:
        result = run(args)
    except (OSError, ValueError, subprocess.CalledProcessError) as error:
        if isinstance(error, subprocess.CalledProcessError) and error.stderr:
            print(error.stderr, end="", file=sys.stderr)
        print(f"profile_calls: {error}", file=sys.stderr)
        return 1
    print(json.dumps(result, indent=2).replace(str(Path.home()), "~"))
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
