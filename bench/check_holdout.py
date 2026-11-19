#!/usr/bin/env python3
"""Run correctness holdouts without collecting performance data."""
import argparse
from pathlib import Path
from holdout_cases import CASES
from run_bench import run_once


def main():
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument('--candy', required=True, type=Path)
    ap.add_argument('--out-dir', type=Path, default=Path('build/holdout'))
    args = ap.parse_args()
    binary = args.candy.resolve(strict=True)
    args.out_dir.mkdir(parents=True, exist_ok=True)
    for name, source, expected in CASES:
        path = args.out_dir / (name + '.candy')
        path.write_text(source)
        code, out, err, _ = run_once([str(binary), str(path)])
        if code or err or out != expected:
            raise SystemExit(f'{name}: exit={code}, stdout={out!r}, stderr={err!r}; expected={expected!r}')
        print(name + ': PASS')


if __name__ == '__main__':
    main()
