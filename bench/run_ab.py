#!/usr/bin/env python3
"""Alternate two Candy binaries on identical sources; reject invalid executions.

This measures end-to-end process time, INCLUDING lexing/codegen, not pure VM time.
Build both variants with the same compiler/options; change only the tested feature.
"""
import argparse
import hashlib
import json
import random
import statistics
from pathlib import Path

from gen_cases import build, CASES
from run_bench import run_once


def checked(binary, path, case):
    code, out, err, elapsed = run_once([str(binary), str(path)])
    expected = case['expected'] + ('\n' if case['expected'] else '')
    if code or err or out != expected:
        raise RuntimeError(f"{case['name']} {binary.name}: exit={code}, stdout={out!r}, stderr={err!r}")
    return elapsed


def fingerprint(path):
    return {'name': path.name, 'sha256': hashlib.sha256(path.read_bytes()).hexdigest(),
            'bytes': path.stat().st_size}


def main():
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument('--a', required=True, type=Path)
    ap.add_argument('--b', required=True, type=Path)
    ap.add_argument('--filter', default='', help='comma-separated exact case names; empty is always included')
    ap.add_argument('--reps', type=int, default=15)
    ap.add_argument('--warmup', type=int, default=2)
    ap.add_argument('--seed', type=int, default=1729)
    ap.add_argument('--out', required=True, type=Path)
    ap.add_argument('--build-description', required=True, help='compiler/options and the single intended difference')
    args = ap.parse_args()
    if args.reps < 1 or args.warmup < 0:
        ap.error('reps must be positive and warmup nonnegative')
    binaries = {'a': args.a.resolve(strict=True), 'b': args.b.resolve(strict=True)}
    selected = set(filter(None, args.filter.split(',')))
    cases = build()
    if selected:
        missing = selected - {c['name'] for c in cases}
        if missing:
            ap.error(f'unknown cases: {sorted(missing)}')
        cases = [c for c in cases if c['name'] in selected or c['name'] == 'empty']
    CASES.mkdir(exist_ok=True)
    for case in cases:
        path = CASES / (case['name'] + '.candy')
        path.write_text(case['candy'])
        for binary in binaries.values():
            for _ in range(args.warmup + 1):
                checked(binary, path, case)
    raw = {c['name']: {'a': [], 'b': []} for c in cases}
    rng = random.Random(args.seed)
    for rep in range(args.reps):
        ordered = cases.copy()
        rng.shuffle(ordered)
        for i, case in enumerate(ordered):
            order = ('a', 'b') if (rep + i) % 2 == 0 else ('b', 'a')
            for label in order:
                raw[case['name']][label].append(checked(binaries[label], CASES / (case['name'] + '.candy'), case))
    rows = []
    for case in cases:
        samples = raw[case['name']]
        a, b = (statistics.median(samples[k]) / 1e6 for k in ('a', 'b'))
        ratios = [x / y for x, y in zip(samples['a'], samples['b'])]
        boots = sorted(statistics.median(rng.choices(ratios, k=len(ratios))) for _ in range(2000))
        row = {'name': case['name'], 'a_ms': a, 'b_ms': b, 'ratio_medians': a / b,
               'paired_ratio_median': statistics.median(ratios),
               'paired_bootstrap_95': [boots[50], boots[1949]],
               'source_sha256': hashlib.sha256(case['candy'].encode()).hexdigest()}
        rows.append(row)
        print(f"{case['name']:<22} A={a:9.3f} B={b:9.3f} A/B={a/b:.3f}", flush=True)
    result = {'measurement': 'end-to-end, includes compilation; raw paired samples, no startup subtraction',
              'build_description': args.build_description, 'binaries': {k: fingerprint(p) for k, p in binaries.items()},
              'seed': args.seed, 'reps': args.reps, 'warmup': args.warmup, 'rows': rows, 'raw_ns': raw}
    args.out.parent.mkdir(parents=True, exist_ok=True)
    args.out.write_text(json.dumps(result, indent=2) + '\n')


if __name__ == '__main__':
    main()
