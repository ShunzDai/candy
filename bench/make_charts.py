#!/usr/bin/env python3
"""Generate the two final Candy-vs-Lua comparison charts."""
import argparse
import json
import math
from pathlib import Path

from PIL import Image, ImageDraw, ImageFont

ROOT = Path(__file__).resolve().parent
OUT = ROOT / "charts"
OUT.mkdir(exist_ok=True)
S = 2
W = 1400 * S

CANDY = "#E07B39"
LUA = "#3C6E9F"
GOOD = "#3E8E5A"
BAD = "#C8553D"
TEXT = "#1F2933"
MUTED = "#6B7280"
GRID = "#E3E6EA"
BG = "#FFFFFF"
FONT = "/System/Library/Fonts/Hiragino Sans GB.ttc"
F_TITLE = ImageFont.truetype(FONT, 40 * S)
F_SUB = ImageFont.truetype(FONT, 21 * S)
F_LABEL = ImageFont.truetype(FONT, 23 * S)
F_TICK = ImageFont.truetype(FONT, 19 * S)
F_VALUE = ImageFont.truetype(FONT, 20 * S)


def canvas(height):
    img = Image.new("RGB", (W, height * S), BG)
    return img, ImageDraw.Draw(img)


def save(img, name):
    img.resize((img.width // S, img.height // S), Image.LANCZOS).save(OUT / name)
    print("saved", OUT / name)


def text(d, xy, value, font, fill=TEXT, anchor="la"):
    d.text((xy[0] * S, xy[1] * S), value, font=font, fill=fill, anchor=anchor)


def line(d, pts, fill, width=2):
    d.line([(x * S, y * S) for x, y in pts], fill=fill, width=width * S)


def rect(d, box, fill, radius=0):
    d.rounded_rectangle([box[0] * S, box[1] * S, box[2] * S, box[3] * S], radius=radius * S, fill=fill)


def title(d, main, sub):
    text(d, (60, 42), main, F_TITLE)
    text(d, (60, 100), sub, F_SUB, MUTED)


def load(time_path, memory_path):
    rows = {r["name"]: r for r in json.loads(time_path.read_text())["rows"]}
    memory = json.loads(memory_path.read_text())
    return rows, memory


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
        ("for 局部累加 50 万次", "for_local_500k"),
        ("循环全局函数调用 50 万次", "calls_hot_global_500k"),
        ("循环局部函数调用 50 万次", "calls_hot_local_500k"),
        ("循环参数函数调用 50 万次", "calls_hot_parameter_500k"),
        ("if / elif / else 50 万次", "branch_chain_500k"),
        ("break / continue 循环", "break_continue_500k"),
        ("循环嵌套 def 绑定 1000 次 *", "nested_functions_1k"),
    ]),
    ("递归与尾调用", [
        ("fib(34)", "fib_34"),
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


def chart_time(rows, candy_label, lua_label):
    count = sum(len(items) for _, items in TIME_GROUPS)
    height = 210 + 48 * count + 58 * len(TIME_GROUPS) + 80
    img, d = canvas(height)
    title(d, "Candy vs Lua：扩展基准耗时", f"{candy_label} / {lua_label}；低于 1 表示 Candy 更快")
    x0, x1, top = 540, 1190, 190
    values = [rows[key]["ratio_median"] for _, items in TIME_GROUPS for _, key in items]
    vmax = max(1.5, math.ceil(max(values) * 2) / 2)
    xv = lambda v: x0 + (x1 - x0) * v / vmax
    bottom = height - 100
    for i in range(7):
        tick = vmax * i / 6
        line(d, [(xv(tick), top - 15), (xv(tick), bottom)], GRID, 1)
        text(d, (xv(tick), bottom + 15), f"{tick:.1f}×", F_TICK, MUTED, "ma")
    line(d, [(xv(1), top - 15), (xv(1), bottom)], "#B0B7C3", 2)
    y = top
    for heading, items in TIME_GROUPS:
        text(d, (60, y), heading, F_LABEL, MUTED)
        y += 48
        for label, key in items:
            value = rows[key]["ratio_median"]
            color = GOOD if value < 1 else (CANDY if value < 1.2 else BAD)
            text(d, (x0 - 18, y + 6), label, F_LABEL, TEXT, "ra")
            rect(d, (x0, y, xv(value), y + 30), color, 4)
            text(d, (xv(value) + 12, y + 5), f"{value:.2f}×", F_VALUE, color)
            y += 48
        y += 10
    text(d, (60, height - 36), "端到端中位数；不扣启动。热循环不等于纯执行计时；详细 ms、吞吐和波动见 JSON/CSV。", F_SUB, MUTED)
    text(d, (60, height - 66), "* 嵌套 def：Candy 复用编译期 closure，Lua 运行时创建；不是等价分配压力。", F_TICK, MUTED)
    save(img, "candy_vs_lua_time.png")


def chart_memory(memory, candy_label, lua_label):
    height = 1220
    img, d = canvas(height)
    title(d, "Candy vs Lua：扩展基准峰值内存", f"{candy_label} / {lua_label}；maximum RSS（MiB）")
    rect(d, (1040, 134, 1064, 152), CANDY, 3)
    text(d, (1074, 132), "Candy", F_TICK)
    rect(d, (1170, 134, 1194, 152), LUA, 3)
    text(d, (1204, 132), "Lua 5.4", F_TICK)
    for panel, (heading, items) in enumerate(MEMORY_GROUPS):
        top = 225 + panel * 475
        bottom = top + 330
        text(d, (60, top - 40), heading, F_LABEL, MUTED)
        x0, x1 = 150, 1300
        maximum = max(memory[key][lang]["rss_bytes"] / 2**20 for _, key in items for lang in ("candy", "lua"))
        # Shared linear scale within each panel; small cases no longer compressed
        # against a 100 MiB ceiling. Panel ranges deliberately differ.
        hi = math.ceil(maximum * 1.15)
        yv = lambda v: bottom - v / hi * (bottom - top)
        for j in range(6):
            tick = hi * j / 5
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
            text(d, (cx - bw / 2 - 4, yv(candy) - 26), f"{candy:.1f}", F_TICK, CANDY, "ma")
            text(d, (cx + bw / 2 + 4, yv(lua) - 26), f"{lua:.1f}", F_TICK, LUA, "ma")
            text(d, (cx, bottom + 18), label, F_TICK, TEXT, "ma")
    text(d, (60, height - 40), "每项 3 次独立进程峰值取中位数；两个面板使用不同线性刻度，不能直接比较跨面板柱高。", F_SUB, MUTED)
    save(img, "candy_vs_lua_memory.png")


def main():
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument('--time', type=Path, default=ROOT / 'final_stack.json')
    ap.add_argument('--memory', type=Path, default=ROOT / 'memory_stack.json')
    ap.add_argument('--candy-label', default='栈 VM · static · O3/LTO')
    ap.add_argument('--lua-label', default='Lua 5.4.7 · static · O3/LTO')
    args = ap.parse_args()
    rows, memory = load(args.time, args.memory)
    chart_time(rows, args.candy_label, args.lua_label)
    chart_memory(memory, args.candy_label, args.lua_label)


if __name__ == "__main__":
    main()
