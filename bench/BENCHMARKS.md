# 当前扩展基准与图表

当前图表已切换到双方匹配构建：同 AppleClang、static、O3/LTO、non-PGO，Candy Release 对照官方 Lua 5.4.7 源码构建；并已包含局部原地加法融合。完整参数与最新数值见 [匹配构建](MATCHED_BUILD.md)。本页下方“本轮观察”保留此前 Homebrew 动态 stock Lua 的历史结果，不再代表当前图片。

当前 34 个时间用例（原 23 + 新 11），12 个峰值内存用例（含 empty）。两张图仍只比较 Candy/Lua：时间图按负载类型分组，内存图分为端到端和热循环两个线性刻度面板，跨面板柱高不能直接比较。

## 用例源码

用例源码放在 `bench/scripts/`（每个模板一对 `.candy` + `.lua`，用 `$name` 占位参数）；`gen_cases.py` 只保留尺寸、期望值与测量元数据，展开后写入 `bench/cases/`（生成物，不入库）。改脚本直接改模板即可，`python3 bench/gen_cases.py` 重新生成。

## 新增覆盖

| 用例 | 测量内容 |
|---|---|
| calls_nested_local_200k | 双方在外层函数中定义局部 id，展开 20 万调用点 |
| while_global_1m / while_local_1m | 全局/函数局部变量循环计数及校验和 |
| for_local_500k | C 风格 for 累加，Lua 降为 while，不使用专门 numeric-for 指令 |
| calls_hot_global/local/parameter_500k | 热循环 while 中进行 50 万次调用并累加返回值 |
| branch_chain_500k | if/elif/else 走不同路径 |
| break_continue_500k | for 的 continue 步进和 break；Lua 用 goto label 保持对应控制流 |
| tail_self_100k | 局部函数自尾递归，双参数累加 |
| nested_functions_1k | 循环执行嵌套 def 绑定；**分配语义不等价**，Candy 复用编译期 closure，Lua 运行时创建 |

每个新增用例 7 次计时；全套 2 次预热，原用例保留其 5–31 次计时。预期输出由 Python 的 Fibonacci、闭式求和或分段计数独立计算；运行时逐字校验 stdout、stderr、退出码。Lua 生成代码另经 luac 检查。

## 指标与口径

- `ratio_median` 为端到端中位数比例（图表主指标），包含进程启动、读取、解析、codegen、执行和回收。
- 热循环减小编译比例，但**不是预编译纯执行计时**。
- JSON 保留原始 ns 样本、min/p10/median/mean/p90/max/stddev、双方二进制 SHA-256。
- 新增源码字节数、负载分类、绑定对称性注记、端到端 ops/s 与 ns/op。不同 unit（call/iteration/def）不能直接相互比较。
- 历史 `net_*` 和 `ops_per_s` 保留兼容，是扣独立空脚本启动的估计，不用作图表或纯执行结论。
- 旧 calls_local_200k 仍是 Candy 全局 / Lua 局部，保留历史但标注 asymmetric，图表改用真正双方局部的新用例。
- 内存为 `/usr/bin/time -l` 每项 1 次预热、3 个进程的 maximum RSS 中位数，保存所有峰值样本；不是精确分项堆用量。

## 本轮观察（端到端）

| 新负载 | Candy / Lua |
|---|---:|
| 双方局部展开调用 | 1.12× |
| while 全局 | 1.21× |
| while 局部 | 0.92× |
| for 局部 | 1.07× |
| 循环全局调用 | 1.32× |
| 循环局部调用 | 1.22× |
| 循环函数参数调用 | 1.21× |
| 分支链 | 0.90× |
| break/continue | 1.08× |
| 自尾递归 | 0.85× |

这些数据表明：局部作用域省下全局读写成本，但热调用仍有约 21–32% 的端到端差距，不能再仅用“编译占比大”解释。嵌套 def 绑定虽更快，但不用于声称分配器/GC 更好。

内存方面，端到端局部调用约 5.5 MiB vs 7.0 MiB，热循环/热调用约 1.3 MiB vs 1.6 MiB。定义 10 万函数 Candy 约 44.1 MiB、Lua 42.8 MiB，存在小幅劣势，不能概括为所有内存项目都赢。

## 复现

```bash
cmake --build build -j4
build/test/test --gtest_brief=1
python3 -m unittest discover -s bench -p 'test_*.py'
python3 bench/run_bench.py --candy build/shell/candy --warmup 2 --out final_stack.json
python3 bench/measure_mem.py --candy build/shell/candy --reps 3 --warmup 1 --out memory_stack.json
python3 bench/make_charts.py
```

Pillow 需在运行图表脚本的 Python 中可用。数据与图表：`final_stack.json/csv`、`memory_stack.json`、`charts/candy_vs_lua_time.png`、`charts/candy_vs_lua_memory.png`。时间和内存数据应使用同一 Candy 二进制指纹。
