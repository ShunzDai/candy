# 当前扩展基准与图表

当前图表比较寄存器 VM 版 Candy 与 Lua 5.4.7，双方使用静态语言核心和 O3/LTO。两张耗时图使用相同的用例顺序、比例尺和版式；柱长为 Candy/Lua 中位数之比，右列为各自的毫秒数。两组数据来自不同机器和编译器，不应把差异仅归因于操作系统。

- macOS（Apple M2）：[耗时](charts/macos_candy_vs_lua_time.png)、[峰值内存](charts/macos_candy_vs_lua_memory.png)。保留 `macos_time.json/csv` 和 `macos_memory.json` 的历史测量，只重绘图表。这批数据未记录 Candy 的源码提交，不能视为使用了 WSL 本次重测的修复版本。
- WSL 2（Intel Core i5-12400、Ubuntu / GCC 13.3.0）：[耗时](charts/wsl_candy_vs_lua_time.png)、[峰值内存](charts/wsl_candy_vs_lua_memory.png)，原始样本和汇总见 `wsl_time.json/csv` 和 `wsl_memory.json`。Candy 使用本提交的源码（含已有局部变量直接写回、按需读取赋值左值的修复）；双方均从源码构建。35 项耗时用例各 3 轮预热、15 次正式测量，按中位数绘图并校验退出码和输出。

两种环境各有一张耗时图、一张峰值内存图，均由同一 Python 脚本绘制。耗时比大于 1× 时标橙，达到 1.5× 时标红。内存测量各含 12 项（含空脚本），每项 1 轮预热、3 次正式测量；macOS 用 BSD `time -l`，WSL 用 GNU `time -f %M`，统一绘制峰值 RSS 的 MiB 中位数。两个环境的对应面板共用刻度；同一张内存图的端到端和热循环面板使用不同刻度，跨面板柱高不能直接比较。下方“本轮观察”和 [匹配构建](MATCHED_BUILD.md) 中的栈 VM 结果为历史记录，不代表当前图表。

## 用例源码

用例源码放在 `bench/scripts/`（每个模板一对 `.candy` + `.lua`，用 `$name` 占位参数）；`gen_cases.py` 只保留尺寸、期望值与测量元数据，展开后写入 `bench/cases/`（生成物，不入库）。改脚本直接改模板即可，`python3 bench/gen_cases.py` 重新生成。

## 新增覆盖

| 用例 | 测量内容 |
|---|---|
| calls_nested_local_200k | 双方在外层函数中定义局部 id，展开 20 万调用点 |
| while_global_1m / while_local_1m | 全局/函数局部变量循环计数及校验和 |
| for_local_500k | 历史用例名；Candy 和 Lua 模板均用 while 进行局部累加 |
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
- 内存为 `/usr/bin/time` 测得的每项 3 个进程的 maximum RSS 中位数，保存所有峰值样本；不是精确分项堆用量。

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

## 复现 WSL 测量

把 checkout、构建目录和 Lua 源码放在 WSL 根文件系统（ext4，例如 `/candy-bench.*`；也可用位于该文件系统的 Linux home），不要从 `/mnt/c` 启动跑分。此次重测的可执行文件、生成的测试脚本和结果都在同一 ext4 文件系统，并将测量进程固定在 vCPU 2。数据依然包含本地脚本读取，不包含 Windows 挂载盘的脚本或可执行文件访问。

在该目录备好 `candy/` checkout 和官方 `lua-5.4.7/` 源码后运行。Lua 源码包的 SHA-256 见 [匹配构建](MATCHED_BUILD.md)。

```bash
findmnt -T . -no TARGET,SOURCE,FSTYPE
cmake -S candy -B build -DCMAKE_BUILD_TYPE=Release -DCANDY_STATIC=ON
cmake --build build --target candy --parallel
make -C lua-5.4.7/src -j8 lua \
  'CC=gcc -std=gnu99' \
  'CFLAGS=-O3 -DNDEBUG -flto -falign-functions=32 -falign-loops=32 -Wall -Wextra -DLUA_COMPAT_5_3 -DLUA_USE_LINUX' \
  'LDFLAGS=-flto -Wl,--gc-sections' 'LIBS=-lm -ldl'
taskset -c 2 python3 candy/bench/run_bench.py --candy "$PWD/build/shell/candy" \
  --lua "$PWD/lua-5.4.7/src/lua" --warmup 3 --reps 15 --out wsl_time.json
taskset -c 2 python3 candy/bench/measure_mem.py --candy "$PWD/build/shell/candy" \
  --lua "$PWD/lua-5.4.7/src/lua" --warmup 1 --reps 3 --out wsl_memory.json
```

测量完成后才复制 `candy/bench/wsl_time.{json,csv}` 和 `candy/bench/wsl_memory.json` 回仓库。使用带 Pillow 的 Python 从已保存数据统一重绘四张图；`--font` 可指定简体中文字体，要复用本次的字形可指定 Noto Sans SC。

```bash
python3 bench/make_charts.py
```
