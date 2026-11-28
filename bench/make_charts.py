#!/usr/bin/env python3
"""Render the published Candy-vs-Lua charts from saved measurements."""
import argparse
import json
import math
import os
from pathlib import Path

from PIL import Image, ImageDraw, ImageFont

ROOT = Path(__file__).resolve().parent
OUT = ROOT / "charts"
S = 2
W = 1400 * S

CANDY = "#E07B39"
LUA = "#3C6E9F"
GOOD = "#3E8E5A"
SEVERE = "#C8553D"
SEVERE_RATIO = 1.5
TEXT = "#1F2933"
MUTED = "#6B7280"
GRID = "#E3E6EA"
BG = "#FFFFFF"
FONT_CANDIDATES = (
    "/usr/share/fonts/opentype/noto/NotoSansCJK-Regular.ttc",
    "/System/Library/Fonts/Hiragino Sans GB.ttc",
    "C:/Windows/Fonts/NotoSansSC-VF.ttf",
    "C:/Windows/Fonts/msyh.ttc",
)


def load_font(path, size, weight):
    font = ImageFont.truetype(str(path), size * S)
    try:
        font.set_variation_by_name(weight)
    except (OSError, ValueError):
        pass  # Static fonts and variable fonts without this named instance.
    return font


def canvas(height):
    img = Image.new("RGB", (W, height * S), BG)
    return img, ImageDraw.Draw(img)


def save(img, path):
    img.resize((img.width // S, img.height // S), Image.LANCZOS).save(path)
    print("saved", path)


def text(d, xy, value, font, fill=TEXT, anchor="la"):
    d.text((xy[0] * S, xy[1] * S), value, font=font, fill=fill, anchor=anchor)


def line(d, pts, fill, width=2):
    d.line([(x * S, y * S) for x, y in pts], fill=fill, width=width * S)


def rect(d, box, fill, radius=0):
    d.rounded_rectangle([box[0] * S, box[1] * S, box[2] * S, box[3] * S], radius=radius * S, fill=fill)


def title(d, main, sub):
    text(d, (60, 42), main, F_TITLE)
    text(d, (60, 100), sub, F_SUB, MUTED)


TIME_GROUPS = [
    ("端到端（编译 + 执行）", [
        ("40 万项算术表达式", "arith_400k"),
        ("编译 20 万行", "compile_200k"),
        ("定义 10 万函数", "closures_100k"),
        ("40 万全局读写语句", "globals_400k"),
        ("20 万全局调用语句", "calls_200k"),
        ("20 万局部调用语句（双方局部）", "calls_nested_local_200k"),
    ]),
    ("热循环（微基准）", [
        ("while 全局变量 100 万次", "while_global_1m"),
        ("while 局部变量 100 万次", "while_local_1m"),
        ("while 局部累加 50 万次", "for_local_500k"),
        ("循环全局函数调用 50 万次", "calls_hot_global_500k"),
        ("循环局部函数调用 50 万次", "calls_hot_local_500k"),
        ("循环参数函数调用 50 万次", "calls_hot_parameter_500k"),
        ("if / elif / else 50 万次", "branch_chain_500k"),
        ("break / continue 循环", "break_continue_500k"),
        ("循环嵌套 def 绑定 1000 次 *", "nested_functions_1k"),
    ]),
    ("递归与尾调用", [
        ("fib(34)", "fib_34"),
        ("fib 全局函数", "fib_global_32"),
        ("fib 别名动态调用", "fib_alias_30"),
        ("互递归 41 万次", "mutual_10k"),
        ("局部自尾递归 10 万次", "tail_self_100k"),
    ]),
]

MEMORY_GROUPS = [
    ("端到端峰值 RSS", [
        ("40 万项算术", "arith_400k"),
        ("编译 20 万行", "compile_200k"),
        ("40 万调用点", "calls_400k"),
        ("20 万局部调用", "calls_nested_local_200k"),
        ("40 万全局读写", "globals_400k"),
        ("定义 10 万函数", "closures_100k"),
    ]),
    ("热循环峰值 RSS", [
        ("fib(34)", "fib_34"),
        ("局部 while", "while_local_1m"),
        ("局部热调用", "calls_hot_local_500k"),
        ("分支链", "branch_chain_500k"),
        ("循环建函数", "nested_functions_1k"),
    ]),
]


def chart_time(rows, environment, build_label, path, vmax):
    count = sum(len(items) for _, items in TIME_GROUPS)
    height = 210 + 48 * count + 58 * len(TIME_GROUPS) + 80
    img, d = canvas(height)
    title(d, f"Candy vs Lua · {environment}：端到端耗时", build_label)
    x0, x1, top = 520, 1120, 190
    xv = lambda v: x0 + (x1 - x0) * v / vmax
    bottom = height - 110
    for i in range(round(vmax * 2) + 1):
        tick = i / 2
        line(d, [(xv(tick), top - 15), (xv(tick), bottom)], GRID, 1)
        text(d, (xv(tick), bottom + 15), f"{tick:g}×", F_TICK, MUTED, "ma")
    line(d, [(xv(1), top - 15), (xv(1), bottom)], "#B0B7C3", 2)
    text(d, (x0, 140), f"耗时比（Candy / Lua）；较慢标橙，≥ {SEVERE_RATIO:g}× 标红", F_TICK, MUTED)
    text(d, (1340, 140), "Candy / Lua（ms）", F_TICK, MUTED, "ra")
    y = top
    for heading, items in TIME_GROUPS:
        text(d, (60, y), heading, F_LABEL, MUTED)
        y += 48
        for label, key in items:
            row = rows[key]
            value = row["ratio_median"]
            color = SEVERE if value >= SEVERE_RATIO else CANDY if value > 1 else GOOD if value < 1 else MUTED
            text(d, (x0 - 18, y + 6), label, F_LABEL, TEXT, "ra")
            rect(d, (x0, y, xv(value), y + 30), color, 4)
            text(d, (xv(value) + 12, y + 5), f"{value:.2f}×", F_VALUE)
            text(d, (1340, y + 6), f"{row['candy']['median_ms']:.2f} / {row['lua']['median_ms']:.2f}",
                 F_TICK, MUTED, "ra")
            y += 48
        y += 10
    text(d, (60, height - 65), "端到端中位数包含启动、读取、编译、执行和回收；详细样本见 JSON/CSV。", F_TICK, MUTED)
    text(d, (60, height - 36), "* 嵌套 def：Candy 和 Lua 的闭包分配行为不同。", F_TICK, MUTED)
    save(img, path)


def memory_tick_steps(datasets):
    steps = []
    for _, items in MEMORY_GROUPS:
        maximum = max(memory[key][lang]['rss_bytes'] / 2**20
                      for memory in datasets for _, key in items for lang in ('candy', 'lua'))
        target = maximum * 1.1 / 6
        order = 10 ** math.floor(math.log10(target))
        steps.append(next(n * order for n in (1, 2, 2.5, 5, 10) if n * order >= target))
    return steps


def chart_memory(memory, environment, build_label, path, steps):
    height = 1220
    img, d = canvas(height)
    title(d, f"Candy vs Lua · {environment}：峰值内存（MiB）", build_label)
    rect(d, (1040, 134, 1064, 152), CANDY, 3)
    text(d, (1074, 132), "Candy", F_TICK)
    rect(d, (1170, 134, 1194, 152), LUA, 3)
    text(d, (1204, 132), "Lua 5.4", F_TICK)
    for panel, ((heading, items), step) in enumerate(zip(MEMORY_GROUPS, steps)):
        top = 225 + panel * 475
        bottom = top + 330
        text(d, (60, top - 40), heading, F_LABEL, MUTED)
        x0, x1 = 150, 1300
        # Both environments use the same scale for a given panel.
        hi = step * 6
        yv = lambda v: bottom - v / hi * (bottom - top)
        for j in range(7):
            tick = step * j
            line(d, [(x0, yv(tick)), (x1, yv(tick))], GRID, 1)
            text(d, (x0 - 16, yv(tick)), f"{tick:g}", F_TICK, MUTED, "rm")
        group = (x1 - x0) / len(items)
        bw = 32
        for i, (label, key) in enumerate(items):
            cx = x0 + group * (i + 0.5)
            candy = memory[key]["candy"]["rss_bytes"] / 2**20
            lua = memory[key]["lua"]["rss_bytes"] / 2**20
            rect(d, (cx - bw - 4, yv(candy), cx - 4, bottom), CANDY, 4)
            rect(d, (cx + 4, yv(lua), cx + bw + 4, bottom), LUA, 4)
            text(d, (cx - bw / 2 - 4, yv(candy) - 26), f"{candy:.1f}", F_TICK, TEXT, "ma")
            text(d, (cx + bw / 2 + 4, yv(lua) - 26), f"{lua:.1f}", F_TICK, TEXT, "ma")
            text(d, (cx, bottom + 18), label, F_TICK, TEXT, "ma")
    reps = memory["_metadata"]["config"]["reps"]
    text(d, (60, height - 36), f"每项取 {reps} 次独立进程的峰值 RSS（MiB）中位数；两个面板的线性刻度不同。", F_TICK, MUTED)
    save(img, path)


def find_font(font=None):
    if font is not None:
        font = font.expanduser()
        if not font.is_file():
            raise ValueError(f'font file not found: {font}')
        return font
    candidates = [Path(path) for path in FONT_CANDIDATES]
    if prefix := os.environ.get('PREFIX'):
        fonts = Path(prefix) / 'share' / 'fonts'
        for name in ('NotoSansCJK-Regular.ttc', 'NotoSansSC-Regular.otf', 'NotoSansSC-VF.ttf'):
            candidates.extend(sorted(fonts.rglob(name)))
    for path in candidates:
        if path.is_file():
            return path
    raise ValueError('no CJK font found; pass --font with the path to a Simplified Chinese font')


def render_charts(datasets, out_dir, font=None, *, build_label='静态核心 · O3/LTO'):
    font = find_font(font)
    global F_TITLE, F_SUB, F_LABEL, F_TICK, F_VALUE
    F_TITLE, F_SUB, F_LABEL, F_TICK, F_VALUE = (
        load_font(font, size, weight) for size, weight in
        ((40, 'Medium'), (21, 'Regular'), (23, 'Regular'), (19, 'Regular'), (20, 'Regular'))
    )

    series = []
    for environment, platform_id, time_path, memory_path, time_name, memory_name in datasets:
        data = json.loads(time_path.read_text(encoding='utf-8'))
        memory = json.loads(memory_path.read_text(encoding='utf-8'))
        if any(platform_id not in info['platform'].lower()
               for info in (data['env'], memory['_metadata'])):
            raise ValueError(f'input data do not match the {environment} chart label')
        if data['env']['platform'] != memory['_metadata']['platform']:
            raise ValueError(f'{environment} time and memory measurements used different platforms')
        for lang in ('candy', 'lua'):
            if memory['_metadata']['binaries'][lang]['sha256'] != data['env'][f'{lang}_sha256']:
                raise ValueError(f'{environment} time and memory measurements used different {lang} binaries')

        lua_version = ' '.join(data['env']['lua_version'].split()[:2])
        build = f'Candy 寄存器 VM / {lua_version}'
        if build_label:
            build += ' · ' + build_label
        rows = {r['name']: r for r in data['rows']}
        series.append((rows, memory, environment, build, time_name, memory_name))

    maximum = max(rows[key]['ratio_median'] for rows, *_ in series
                  for _, items in TIME_GROUPS for _, key in items)
    vmax = max(2, math.ceil(maximum * 2) / 2)
    steps = memory_tick_steps([memory for _, memory, *_ in series])
    out_dir.mkdir(parents=True, exist_ok=True)
    for rows, memory, environment, build, time_name, memory_name in series:
        chart_time(rows, environment, build, out_dir / time_name, vmax)
        chart_memory(memory, environment, build, out_dir / memory_name, steps)


def main():
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument('--macos-time', type=Path, default=ROOT / 'macos_time.json')
    ap.add_argument('--macos-memory', type=Path, default=ROOT / 'macos_memory.json')
    ap.add_argument('--wsl-time', type=Path, default=ROOT / 'wsl_time.json')
    ap.add_argument('--wsl-memory', type=Path, default=ROOT / 'wsl_memory.json')
    ap.add_argument('--font', type=Path, help='path to a Simplified Chinese font (TTF/OTF/TTC)')
    ap.add_argument('--out-dir', type=Path, default=OUT)
    args = ap.parse_args()

    datasets = (
        ('macOS', 'macos-', args.macos_time, args.macos_memory,
         'macos_candy_vs_lua_time.png', 'macos_candy_vs_lua_memory.png'),
        ('WSL 2', 'wsl2', args.wsl_time, args.wsl_memory,
         'wsl_candy_vs_lua_time.png', 'wsl_candy_vs_lua_memory.png'),
    )
    try:
        render_charts(datasets, args.out_dir, args.font)
    except (OSError, ValueError, KeyError) as error:
        ap.error(str(error))


if __name__ == "__main__":
    main()
