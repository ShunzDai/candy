#!/usr/bin/env python3
"""Run the Candy vs Lua benchmark suite and emit results.json / results.csv."""
import argparse
import csv
import hashlib
import json
import math
import os
import platform
import shutil
import statistics
import subprocess
import sys
import time
from datetime import datetime, timezone
from pathlib import Path

from gen_cases import CASES as CASES_DIR, build

ROOT = Path(__file__).resolve().parent
REPO = ROOT.parent
CANDY = Path(os.environ.get("CANDY_BIN", REPO / "build" / "shell" / "candy"))
LUA = shutil.which("lua") or "/opt/homebrew/bin/lua"


def cmd(lang: str, case: dict) -> list[str]:
    if lang == "candy":
        return [str(CANDY), str(CASES_DIR / f"{case['name']}.candy")]
    return [str(LUA), str(CASES_DIR / f"{case['name']}.lua")]


def sanitize(text: str) -> str:
    """Keep local home directory names out of reports and diagnostics."""
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
        raise argparse.ArgumentTypeError("must be a finite positive number of seconds")
    return number


def executable(value: str, name: str) -> str:
    resolved = shutil.which(os.path.expanduser(value))
    if resolved is None:
        raise SystemExit(sanitize(f"{name} executable not found or not executable: {value}"))
    return str(Path(resolved).resolve())


def run_once(argv: list[str], timeout: float = 120.0) -> tuple[int, str, str, int]:
    t0 = time.perf_counter_ns()
    try:
        proc = subprocess.run(argv, stdout=subprocess.PIPE, stderr=subprocess.PIPE, timeout=timeout)
    except subprocess.TimeoutExpired:
        raise SystemExit(sanitize(f"command timed out after {timeout:g}s: {argv!r}")) from None
    except OSError as exc:
        raise SystemExit(sanitize(f"cannot run {argv!r}: {exc}")) from None
    t1 = time.perf_counter_ns()
    return proc.returncode, proc.stdout.decode(errors="replace"), proc.stderr.decode(errors="replace"), t1 - t0


def checked_run(lang: str, case: dict, phase: str, timeout: float) -> int:
    code, out, err, elapsed = run_once(cmd(lang, case), timeout)
    expected = case["expected"] + "\n" if case["expected"] else ""
    if code != 0 or err or out != expected:
        raise SystemExit(sanitize(
            f"{phase} failed: {case['name']} {lang} exit={code} "
            f"out={out!r} expected={expected!r} err={err!r}"))
    return elapsed


def verify(cases: list[dict], timeout: float = 120.0) -> None:
    for case in cases:
        for lang in ("candy", "lua"):
            checked_run(lang, case, "verify", timeout)


def warmup(cases: list[dict], passes: int, timeout: float) -> None:
    for w in range(passes):
        order = ("candy", "lua") if w % 2 == 0 else ("lua", "candy")
        for case in cases:
            for lang in order:
                checked_run(lang, case, "warmup", timeout)


def env_info(timeout: float = 120.0) -> dict:
    info = {
        "timestamp": datetime.now(timezone.utc).isoformat(),
        "platform": platform.platform(),
        "machine": platform.machine(),
        "processor": platform.processor(),
        "python": platform.python_version(),
        "cpu_count": os.cpu_count(),
        "candy": sanitize(str(CANDY)),
        "lua": sanitize(str(LUA)),
    }
    try:
        proc = subprocess.run([LUA, "-v"], capture_output=True, text=True, timeout=timeout)
        if proc.returncode == 0:
            info["lua_version"] = sanitize((proc.stdout or proc.stderr).strip())
    except (OSError, subprocess.TimeoutExpired):
        pass
    for key, path in (
        ("candy_bin_bytes", CANDY),
        ("lua_bin_bytes", Path(LUA).resolve()),
    ):
        try:
            info[key] = path.stat().st_size
        except OSError:
            pass
    for lang, path in (("candy", CANDY), ("lua", Path(LUA))):
        if path.is_file():
            info[f"{lang}_sha256"] = hashlib.sha256(path.read_bytes()).hexdigest()
    if sys.platform == "darwin":
        for key, argv in (("cpu_brand", ["sysctl", "-n", "machdep.cpu.brand_string"]),
                          ("os", ["sw_vers", "-productVersion"])):
            try:
                proc = subprocess.run(argv, capture_output=True, text=True, timeout=timeout)
                if proc.returncode == 0:
                    info[key] = sanitize(proc.stdout.strip())
            except (OSError, subprocess.TimeoutExpired):
                pass
    return info


def summarize(times: list[int]) -> dict:
    s = sorted(times)
    n = len(s)
    return {
        "n": n,
        "min_ms": s[0] / 1e6,
        "p10_ms": s[max(0, int(0.10 * (n - 1)))] / 1e6,
        "median_ms": statistics.median(s) / 1e6,
        "mean_ms": statistics.fmean(s) / 1e6,
        "p90_ms": s[min(n - 1, int(0.90 * (n - 1)))] / 1e6,
        "max_ms": s[-1] / 1e6,
        "stdev_ms": statistics.pstdev(s) / 1e6 if n > 1 else 0.0,
    }


def main() -> None:
    global CANDY, LUA
    ap = argparse.ArgumentParser()
    ap.add_argument("--reps", type=positive_int, help="override repetitions for every case")
    ap.add_argument("--warmup", type=nonnegative_int, default=2)
    ap.add_argument("--timeout", type=positive_timeout, default=120.0,
                    help="per-process timeout in seconds (default: 120)")
    ap.add_argument("--filter", default="")
    ap.add_argument("--skip-verify", action="store_true")
    ap.add_argument("--candy", default=str(CANDY), help="candy executable to benchmark")
    ap.add_argument("--lua", default=str(LUA), help="lua executable to benchmark")
    ap.add_argument("--out", default="results.json", help="output json path")
    args = ap.parse_args()

    CANDY = Path(executable(args.candy, "candy"))
    LUA = executable(args.lua, "lua")

    cases = build()
    # write fresh sources
    CASES_DIR.mkdir(exist_ok=True)
    for case in cases:
        (CASES_DIR / f"{case['name']}.candy").write_text(case["candy"])
        (CASES_DIR / f"{case['name']}.lua").write_text(case["lua"])
    if args.filter:
        cases = [c for c in cases if args.filter in c["name"] or c["name"] == "empty"]
    if not cases:
        raise SystemExit("no cases selected")

    if not args.skip_verify:
        verify(cases, args.timeout)

    raw: dict[str, dict[str, list[int]]] = {c["name"]: {"candy": [], "lua": []} for c in cases}

    warmup(cases, args.warmup, args.timeout)
    print(f"warmup done ({args.warmup} passes), measuring...", flush=True)

    start = time.perf_counter()
    max_reps = max((args.reps or c["reps"]) for c in cases)
    for rep in range(max_reps):
        for idx, case in enumerate(cases):
            reps = args.reps or case["reps"]
            if rep >= reps:
                continue
            order = ("candy", "lua") if (rep + idx) % 2 == 0 else ("lua", "candy")
            for lang in order:
                dt = checked_run(lang, case, "run", args.timeout)
                raw[case["name"]][lang].append(dt)
    elapsed = time.perf_counter() - start

    empty_med = {lang: statistics.median(raw["empty"][lang]) for lang in ("candy", "lua")}

    rows = []
    for case in cases:
        stats = {lang: summarize(raw[case["name"]][lang]) for lang in ("candy", "lua")}
        net = {lang: max(1e-9, stats[lang]["median_ms"] - empty_med[lang] / 1e6) for lang in ("candy", "lua")}
        row = {
            "name": case["name"],
            "group": case["group"],
            "work": case["work"],
            "unit": case["unit"],
            "measurement_kind": case.get("measurement_kind", "end_to_end"),
            "binding": case.get("binding", "see_source"),
            "binding_note": case.get("binding_note", ""),
            "source_bytes": {lang: len(case[lang].encode()) for lang in ("candy", "lua")},
            "candy": stats["candy"],
            "lua": stats["lua"],
            "net_candy_ms": net["candy"],
            "net_lua_ms": net["lua"],
            "ratio_median": stats["candy"]["median_ms"] / stats["lua"]["median_ms"],
            "ratio_net": net["candy"] / net["lua"],
        }
        if case["work"]:
            row["candy_ops_per_s"] = case["work"] / (net["candy"] / 1e3) if net["candy"] > 0 else 0
            row["lua_ops_per_s"] = case["work"] / (net["lua"] / 1e3) if net["lua"] > 0 else 0
            for lang in ("candy", "lua"):
                row[f"{lang}_end_to_end_ops_per_s"] = case["work"] / (stats[lang]["median_ms"] / 1e3)
                row[f"{lang}_end_to_end_ns_per_op"] = stats[lang]["median_ms"] * 1e6 / case["work"]
        rows.append(row)

    result = {
        "env": env_info(args.timeout),
        "measurement": {
            "warmup_passes": args.warmup,
            "reps_override": args.reps,
            "elapsed_s": elapsed,
            "startup_median_ms": {"candy": empty_med["candy"] / 1e6, "lua": empty_med["lua"] / 1e6},
            "scope": "end-to-end including startup, parse/codegen, execution and cleanup; hot loops are not precompiled execution",
            "primary_ratio": "ratio_median; net ratios are startup-subtracted estimates only",
        },
        "rows": rows,
        "raw_ns": raw,
    }
    (ROOT / args.out).write_text(json.dumps(result, indent=2))

    with (ROOT / args.out.replace(".json", ".csv")).open("w", newline="") as f:
        writer = csv.writer(f, lineterminator="\n")
        writer.writerow(["case", "group", "work", "unit",
                         "candy_median_ms", "lua_median_ms", "ratio_median",
                         "candy_net_ms", "lua_net_ms", "ratio_net",
                         "candy_ops_s", "lua_ops_s", "measurement_kind", "binding",
                         "candy_source_bytes", "lua_source_bytes", "candy_end_to_end_ops_s", "lua_end_to_end_ops_s",
                         "candy_end_to_end_ns_per_op", "lua_end_to_end_ns_per_op"])
        for r in rows:
            writer.writerow([r["name"], r["group"], r["work"], r["unit"],
                             f"{r['candy']['median_ms']:.4f}", f"{r['lua']['median_ms']:.4f}",
                             f"{r['ratio_median']:.3f}",
                             f"{r['net_candy_ms']:.4f}", f"{r['net_lua_ms']:.4f}",
                             f"{r['ratio_net']:.3f}",
                             f"{r.get('candy_ops_per_s', 0):.0f}", f"{r.get('lua_ops_per_s', 0):.0f}",
                             r['measurement_kind'], r['binding'], r['source_bytes']['candy'], r['source_bytes']['lua'],
                             f"{r.get('candy_end_to_end_ops_per_s', 0):.0f}", f"{r.get('lua_end_to_end_ops_per_s', 0):.0f}",
                             f"{r.get('candy_end_to_end_ns_per_op', 0):.2f}", f"{r.get('lua_end_to_end_ns_per_op', 0):.2f}"])

    print(f"\n{'case':<18}{'candy ms':>10}{'lua ms':>10}{'ratio':>8}{'candy net':>11}{'lua net':>10}{'net ratio':>10}")
    for r in rows:
        print(f"{r['name']:<18}{r['candy']['median_ms']:>10.3f}{r['lua']['median_ms']:>10.3f}"
              f"{r['ratio_median']:>8.2f}{r['net_candy_ms']:>11.3f}{r['net_lua_ms']:>10.3f}"
              f"{r['ratio_net']:>10.2f}")
    print(f"\nstartup(empty) median: candy {empty_med['candy'] / 1e6:.3f} ms, "
          f"lua {empty_med['lua'] / 1e6:.3f} ms")
    print(sanitize(f"wall time {elapsed:.1f}s, results -> {ROOT / args.out}"))


if __name__ == "__main__":
    main()
