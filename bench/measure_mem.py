#!/usr/bin/env python3
"""Measure median per-process peak RSS using macOS /usr/bin/time -l.

Every warmup and measured process must produce the independently generated
expected stdout, with no diagnostics. Raw peak samples are retained for audits.
"""
import argparse
import hashlib
import json
import math
import os
import platform
import re
import shutil
import statistics
import subprocess
from datetime import datetime, timezone
from pathlib import Path

from gen_cases import CASES as CASES_DIR, build

ROOT = Path(__file__).resolve().parent
REPO = ROOT.parent
CANDY = Path(os.environ.get("CANDY_BIN", REPO / "build" / "shell" / "candy"))
LUA = shutil.which("lua") or "/opt/homebrew/bin/lua"
CASES = ["empty", "fib_34", "arith_400k", "globals_400k", "calls_400k",
         "compile_200k", "closures_100k", "calls_nested_local_200k", "while_local_1m",
         "calls_hot_local_500k", "branch_chain_500k", "nested_functions_1k"]
TIME = "/usr/bin/time"
# Only known complete time(1) lines may be removed from stderr. A loose numeric
# prefix or a substring search could hide a zero-exit compiler diagnostic.
TIME_LABELS = (
    "maximum resident set size", "average shared memory size",
    "average unshared data size", "average unshared stack size",
    "page reclaims", "page faults", "swaps", "block input operations",
    "block output operations", "messages sent", "messages received",
    "signals received", "voluntary context switches", "involuntary context switches",
    "instructions retired", "cycles elapsed", "peak memory footprint",
)
STAT_LINE = re.compile(r"\s*(\d+)\s+(" + "|".join(TIME_LABELS) + r")\s*")
TIMING_LINE = re.compile(r"\s*\d+(?:\.\d+)?\s+real\s+\d+(?:\.\d+)?\s+user\s+\d+(?:\.\d+)?\s+sys\s*")


def sanitize(text: str) -> str:
    return text.replace(str(Path.home()), "~")


def positive_int(value: str) -> int:
    number = int(value)
    if number <= 0:
        raise argparse.ArgumentTypeError("must be a positive integer")
    return number


def nonnegative_int(value: str) -> int:
    number = int(value)
    if number < 0:
        raise argparse.ArgumentTypeError("must be a nonnegative integer")
    return number


def positive_timeout(value: str) -> float:
    number = float(value)
    if not math.isfinite(number) or number <= 0:
        raise argparse.ArgumentTypeError("must be finite and positive")
    return number


def executable(value: str, name: str) -> Path:
    resolved = shutil.which(os.path.expanduser(value))
    if resolved is None:
        raise SystemExit(sanitize(f"{name} executable not found or not executable: {value}"))
    return Path(resolved).resolve()


def peak_rss(argv: list[str], expected: str, timeout: float = 120.0) -> tuple[int, int]:
    """Validate the full process output and parse only complete time-stat lines."""
    try:
        proc = subprocess.run([TIME, "-l", *argv], capture_output=True, timeout=timeout,
                              env={**os.environ, "LC_ALL": "C"})
    except subprocess.TimeoutExpired:
        raise SystemExit(sanitize(f"command timed out after {timeout:g}s: {argv!r}")) from None
    except OSError as exc:
        raise SystemExit(sanitize(f"cannot run {argv!r}: {exc}")) from None

    stats = {}
    diagnostics = []
    timing_lines = 0
    for line in proc.stderr.decode(errors="replace").splitlines():
        match = STAT_LINE.fullmatch(line)
        if match and match[2] not in stats:
            stats[match[2]] = int(match[1])
        elif TIMING_LINE.fullmatch(line) and timing_lines == 0:
            timing_lines += 1
        elif line.strip():
            diagnostics.append(line)
    expected_bytes = (expected + "\n" if expected else "").encode()
    rss = stats.get("maximum resident set size", 0)
    if (proc.returncode != 0 or proc.stdout != expected_bytes or diagnostics
            or timing_lines != 1 or rss <= 0):
        raise SystemExit(sanitize(
            f"measurement failed: {argv!r} exit={proc.returncode} "
            f"out={proc.stdout!r} expected={expected_bytes!r} "
            f"err={proc.stderr.decode(errors='replace')!r}"))
    return rss, stats.get("peak memory footprint", 0)


def binary_info(path: Path) -> dict:
    digest = hashlib.sha256()
    with path.open("rb") as source:
        for chunk in iter(lambda: source.read(1024 * 1024), b""):
            digest.update(chunk)
    return {"path": sanitize(str(path)), "sha256": digest.hexdigest(),
            "size_bytes": path.stat().st_size}


def main() -> None:
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument("--candy", default=str(CANDY), help="Candy executable")
    ap.add_argument("--lua", default=str(LUA), help="Lua executable")
    ap.add_argument("--out", default="memory.json", help="JSON path, relative to bench/")
    ap.add_argument("--reps", type=positive_int, default=3,
                    help="measured processes per language/case (default: 3)")
    ap.add_argument("--warmup", type=nonnegative_int, default=1,
                    help="discarded passes per language/case (default: 1)")
    ap.add_argument("--timeout", type=positive_timeout, default=120.0,
                    help="per-process timeout in seconds (default: 120)")
    args = ap.parse_args()
    binaries = {"candy": executable(args.candy, "candy"),
                "lua": executable(args.lua, "lua")}
    identities = {lang: binary_info(path) for lang, path in binaries.items()}
    available = {case["name"]: case for case in build()}
    cases = [available[name] for name in CASES]
    CASES_DIR.mkdir(parents=True, exist_ok=True)
    for case in cases:
        for lang in binaries:
            (CASES_DIR / f"{case['name']}.{lang}").write_text(case[lang])

    out = {}
    print(f"{'case':<16}{'candy rss':>14}{'lua rss':>14}{'ratio':>8}")
    for index, case in enumerate(cases):
        samples = {lang: [] for lang in binaries}
        for rep in range(args.warmup + args.reps):
            order = ("candy", "lua") if (index + rep) % 2 == 0 else ("lua", "candy")
            for lang in order:
                rss, peak = peak_rss(
                    [str(binaries[lang]), str(CASES_DIR / f"{case['name']}.{lang}")],
                    case["expected"], args.timeout)
                if rep >= args.warmup:
                    samples[lang].append({"rss_bytes": rss, "peak_bytes": peak})
        res = {}
        for lang in binaries:
            res[lang] = {
                "rss_bytes": statistics.median(s["rss_bytes"] for s in samples[lang]),
                "peak_bytes": statistics.median(s["peak_bytes"] for s in samples[lang]),
                "samples": samples[lang],
            }
        out[case["name"]] = res
        print(f"{case['name']:<16}{res['candy']['rss_bytes'] / 2**20:>12.1f}M"
              f"{res['lua']['rss_bytes'] / 2**20:>12.1f}M"
              f"{res['candy']['rss_bytes'] / res['lua']['rss_bytes']:>8.2f}")

    # Do not attribute stale build-release libraries to the selected executable.
    out["_sizes"] = {f"{lang}_bin": info["size_bytes"] for lang, info in identities.items()}
    out["_metadata"] = {
        "timestamp": datetime.now(timezone.utc).isoformat(),
        "platform": platform.platform(),
        "binaries": identities,
        "config": {"reps": args.reps, "warmup": args.warmup, "timeout": args.timeout,
                   "cases": CASES, "out": sanitize(str((ROOT / args.out).resolve())),
                   "cases_dir": sanitize(str(CASES_DIR)), "time": TIME,
                   "statistic": "median of per-process peaks", "units": "bytes",
                   "order": "alternating candy/lua by case and pass"},
    }
    destination = ROOT / args.out
    destination.write_text(json.dumps(out, indent=2) + "\n")
    print(sanitize(f"saved -> {destination}"))


if __name__ == "__main__":
    main()
