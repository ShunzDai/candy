#!/usr/bin/env python3
"""Measure the current platform and refresh its published benchmark charts."""
import argparse
import platform
import re
import subprocess
import sys
from pathlib import Path
from tempfile import TemporaryDirectory

from measure_mem import check_time
from run_bench import CANDY, LUA, REPO, executable

ROOT = Path(__file__).resolve().parent
KNOWN = {'macos': ('macOS', 'macos-'), 'wsl': ('WSL 2', 'wsl2')}


def environment(system, release, machine):
    if system == 'Darwin':
        return 'macos', *KNOWN['macos']
    if system == 'Linux' and 'wsl2' in release.lower():
        return 'wsl', *KNOWN['wsl']
    wsl = system == 'Linux' and 'microsoft' in release.lower()
    family = 'wsl1' if wsl else system.lower()
    slug = re.sub(r'[^a-z0-9_-]+', '-', f'{family}-{machine}'.lower()).strip('-_') or 'other'
    return slug, f'{"WSL 1" if wsl else system} · {machine}', 'microsoft' if wsl else system.lower() + '-'


def dataset(slug, label, marker, source):
    return (label, marker, source / f'{slug}_time.json', source / f'{slug}_memory.json',
            f'{slug}_candy_vs_lua_time.png', f'{slug}_candy_vs_lua_memory.png')


def refresh(root, current, candy, lua, font, render):
    slug, label, marker = current
    charts = root / 'charts'
    with TemporaryDirectory(prefix='.refresh-', dir=root) as tmp:
        work = Path(tmp)
        entry = dataset(slug, label, marker, work)
        args = ['--candy', candy, '--lua', lua]
        subprocess.run([sys.executable, str(ROOT / 'run_bench.py'), *args,
                        '--warmup', '3', '--reps', '15', '--out', str(entry[2])], check=True)
        subprocess.run([sys.executable, str(ROOT / 'measure_mem.py'), *args,
                        '--warmup', '1', '--reps', '3', '--out', str(entry[3])], check=True)
        datasets = [entry]
        if slug in KNOWN:
            for other, (other_label, other_marker) in KNOWN.items():
                if other != slug:
                    peer = dataset(other, other_label, other_marker, root)
                    if peer[2].is_file() and peer[3].is_file():
                        datasets.append(peer)
        render(datasets, work, font)

        # Do not replace anything until every measurement and chart is ready.
        data = [entry[2], entry[2].with_suffix('.csv'), entry[3]]
        images = [work / name for item in datasets for name in item[4:]]
        for path in data + images:
            if path.stat().st_size == 0:
                raise ValueError(f'empty output: {path.name}')
        charts.mkdir(exist_ok=True)
        for path in data:
            path.replace(root / path.name)
        for path in images:
            path.replace(charts / path.name)


def main():
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument('--candy', default=str(CANDY), help='Candy executable (default: build/shell/candy)')
    bundled_lua = REPO / 'build/lua-matched/lua-5.4.7/src/lua'
    ap.add_argument('--lua', default=str(bundled_lua) if bundled_lua.is_file() else LUA,
                    help='Lua executable (prefer the local matched build, then PATH)')
    ap.add_argument('--font', type=Path, help='path to a Simplified Chinese font')
    args = ap.parse_args()
    try:
        from make_charts import find_font, render_charts
    except ModuleNotFoundError as error:
        if error.name == 'PIL':
            ap.error('Pillow is required; install it in the Python environment running this command')
        raise
    current = environment(platform.system(), platform.release(), platform.machine())
    candy, lua = executable(args.candy, 'Candy'), executable(args.lua, 'Lua')
    try:
        check_time()
        refresh(ROOT, current, candy, lua, find_font(args.font),
                lambda datasets, out, font: render_charts(datasets, out, font, build_label=None))
    except (OSError, ValueError, KeyError, subprocess.CalledProcessError) as error:
        ap.error(str(error))
    print(f'Updated benchmark data and charts for {current[1]}')


if __name__ == '__main__':
    main()
