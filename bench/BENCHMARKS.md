# 当前扩展基准与图表

当前图表比较寄存器 VM 版 Candy 与 Lua 5.4.7，双方使用静态语言核心和 O3/LTO。两张耗时图使用相同的用例顺序、比例尺和版式；柱长为 Candy/Lua 中位数之比，右列为各自的毫秒数。两组数据来自不同机器和编译器，不应把差异仅归因于操作系统。

- macOS（Apple M2 / AppleClang）：[耗时](charts/macos_candy_vs_lua_time.png)、[峰值内存](charts/macos_candy_vs_lua_memory.png)。2026-11-28 11:51（UTC+8）用当前工作树（基于 `d1d22b5`，含未提交优化）重建 Candy 后，与本地匹配构建的 Lua 5.4.7 重测。35 项耗时用例各 3 轮预热、15 次正式测量，原始数据见 `macos_time.json/csv` 和 `macos_memory.json`。
- WSL 2（Intel Core i5-12400、Ubuntu / GCC 13.3.0）：[耗时](charts/wsl_candy_vs_lua_time.png)、[峰值内存](charts/wsl_candy_vs_lua_memory.png)，原始样本和汇总见 `wsl_time.json/csv` 和 `wsl_memory.json`。2026-11-27 19:03（UTC+8）用整理后的工作树（基于 `d1d22b5`，含未提交优化及 VM 操作数、表缓存和编译路径的重构）从源码重建并测量 Candy 和 Lua。35 项耗时用例各 3 轮预热、15 次正式测量，按中位数绘图并校验退出码和输出。

macOS 本轮 35 项中 Candy 有 31 项耗时中位数更低，非空用例的 Candy/Lua 耗时比几何均值为 0.775×。4 个慢项为互递归及三个热调用用例；12 项峰值 RSS 中位数均低于 Lua。

保存的 WSL 数据中，35 项耗时中位数均低于 Lua，非空用例的耗时比几何均值为 0.780×；双方局部的 20 万调用点为 19.77 ms 对 20.85 ms。12 项峰值 RSS 中位数也均低于 Lua。本轮只重绘 WSL 图，未在 WSL 重新测量。

两种环境各有一张耗时图、一张峰值内存图，均由同一 Python 脚本绘制。耗时比大于 1× 时标橙，达到 1.5× 时标红。内存测量各含 12 项（含空脚本），每项 1 轮预热、3 次正式测量；macOS 用 BSD `time -l`，WSL 用 GNU `time -f %M`，统一绘制峰值 RSS 的 MiB 中位数。两个环境的对应面板共用刻度；同一张内存图的端到端和热循环面板使用不同刻度，跨面板柱高不能直接比较。下方“本轮观察”和 [匹配构建](MATCHED_BUILD.md) 中的栈 VM 结果为历史记录，不代表当前图表。

## 一键更新当前环境

先构建要测量的 Candy 和 Lua，再用装有 Pillow 的 Python 运行：

```sh
python3 bench/refresh_charts.py --candy build/shell/candy --lua /path/to/lua
```

命令自动识别 macOS、WSL 2 或其他环境。耗时保存为 `<平台>_time.json` 和 `<平台>_time.csv`，内存保存为 `<平台>_memory.json`；平台名例如 `macos`、`wsl`、`linux-x86_64`。macOS / WSL 2 会利用另一平台的已有数据重绘共用刻度的图表，但不覆盖另一平台的测量数据。其他环境只更新自己的图表。峰值 RSS 在 macOS 使用 BSD `/usr/bin/time`，在 Linux 和 Android / Termux 使用 `PATH` 上的 GNU `time`。Termux 如缺少该工具，可运行 `pkg install time`；图表命名如 `android-aarch64_time.json`。

一键脚本会先检查 `time` 和字体路径，耗时、内存和绘图全部成功后才替换输出。找不到中文字体时可传 `--font /path/to/font`；Termux 也会查找 `$PREFIX/share/fonts` 中的 Noto CJK / SC 字体。只想从已有的 macOS / WSL 2 数据重绘时，运行 `python3 bench/make_charts.py`。

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

## 历史记录：关闭编译器优化的 WSL 对照

以下是此前在同一个 WSL ext4 目录取得的实验数据。两边都使用 GCC 13.3.0，关闭优化、自动内联和 LTO；`NDEBUG` 用于对齐断言开关。最后一轮 Candy 保留整数热路径的显式强制内联，Lua 5.4.7 保留源码中的宏展开。当前 CMake 不再提供这种构建模式；这些结果独立于上面的 Release 数据和图表。

如已保留当时的 Candy 二进制（`noopt/shell/candy`），可按当时的命令复测：

```bash
make -C lua-5.4.7/src -j8 lua 'CC=gcc -std=gnu99' \
  'CFLAGS=-O0 -fno-inline -fno-lto -DNDEBUG -DLUA_COMPAT_5_3 -DLUA_USE_LINUX' \
  'LDFLAGS=-fno-lto' 'LIBS=-lm -ldl'
taskset -c 2 python3 candy/bench/run_bench.py --candy "$PWD/noopt/shell/candy" \
  --lua "$PWD/lua-5.4.7/src/lua" --warmup 2 --reps 7 --out "$PWD/noopt-time.json"
taskset -c 2 python3 candy/bench/measure_mem.py --candy "$PWD/noopt/shell/candy" \
  --lua "$PWD/lua-5.4.7/src/lua" --warmup 1 --reps 5 --out "$PWD/noopt-rss.json"
```

先用 Callgrind 看指令成本，用 Massif 看峰值堆分配，再用不带分析器的运行测时间和峰值 RSS。基准脚本已由上面的命令生成：

```bash
valgrind --tool=callgrind --callgrind-out-file=callgrind.global \
  noopt/shell/candy candy/bench/cases/while_global_1m.candy
callgrind_annotate --auto=no callgrind.global
valgrind --tool=massif --time-unit=B --massif-out-file=massif.compile \
  noopt/shell/candy candy/bench/cases/compile_200k.candy
ms_print massif.compile
```

GCC 13.3.0 的一轮对照：Massif 将编译 20 万行的峰值定位到常量索引表重排。表与 GC 常量池共用探测和负载上限；提高该上限减少了过早扩容，插入也复用未命中探测的空槽。Candy 的峰值 RSS 从 31.5 MiB 降到 17.1 MiB（Lua 15.2 MiB），定义 10 万函数从 52.7 MiB 降到 44.7 MiB（Lua 46.4 MiB）。配对 A/B 中，前者耗时约降 4%，后者约增 7%；全局 `while` 仍耗时 Lua 的 4.88 倍。该结果只说明峰值内存改善，不代表运行时已追平。

后续同口径迭代：18 位非负整数字面量内嵌到原有的 32 位指令，不再占用常量槽。编译 20 万行的峰值 RSS 降至 5.0 MiB（Lua 15.2 MiB）；这一轮 12 个 RSS 用例的中位数均低于 Lua。全局读改写复用同一次探测，直接用于分支的 `<` 不再生成临时布尔值，调用同一原型复用指令视图，栈未扩容时复用原地址。全套计时中，Candy/Lua 耗时比分别为：全局 `while` 3.41、局部 `while` 3.40、`fib(34)` 1.97、编译 20 万行 1.10。速度仍未达到逐项超越的目标。

再做单因素配对比较（`run_ab.py`，固定 vCPU，每项 13–17 对）：表和 GC 常量池仍共用探测，每个槽由一次分类区分未使用、已删除和有效，全局 `while` 耗时下降约 5.0%，Callgrind 执行指令数下降 7.4%。词法器按缓冲片段收集标识符，编译 20 万行和定义 10 万函数的耗时分别下降约 3.4% 和 13.4%。小整数加减内嵌到原有指令，局部 `while` 和 `fib(34)` 分别下降约 17.4% 和 7.3%。VM 只在切换原型时解析全局环境，全局 `while` 额外下降约 4.2%。将活槽检查提前未测到稳定收益，未保留。

保留版本复测（WSL ext4、GCC 13.3.0、计时 9 次、RSS 5 次）：Candy/Lua 的端到端耗时比为全局 `while` 3.48、局部 `while` 2.75、`fib(34)` 1.75、编译 20 万行 1.03。12 个峰值 RSS 用例仍全部低于 Lua；编译 20 万行是 4.9 MiB 对 15.1 MiB，定义 10 万函数是 41.6 MiB 对 46.4 MiB。耗时结论以同轮对照或配对实验为准，不跨轮相除。

保留版本的全局 `while` 在 Callgrind 下执行了 24.93 亿条指令，其中共享哈希探测（含子调用）占 37.0%。这项占比衡量指令成本，不等同于可获得的加速比。

只为全局读写增加了 VM 执行期的 8 槽索引缓存。命中省掉散列和探测；未命中、删除、重排或换表回到共享探测。模块从宿主全局表读到的回退结果不缓存。17 对 A/B 中，全局 `while` 的中位耗时从 190.7 ms 降到 82.4 ms，配对加速比 2.32（95% 自助法区间 2.19–2.41）；热全局调用及全局读写 40 万次的配对耗时分别约降 16.8% 和 8.1%。

Callgrind 确认全局 `while` 的共享探测调用数从 4,000,033 次降到 33 次，执行指令从 24.93 亿降到 12.88 亿。和 Lua 同轮复测（9 次计时）时，全局 `while` 为 79.6 ms 对 49.7 ms，局部 `while` 为 37.1 ms 对 14.0 ms。12 项峰值 RSS 的中位数仍全部低于 Lua（5 次采样），其中编译 20 万行为 5.0 MiB 对 15.2 MiB。

仅对整数加法、比较及所用的 `wrap` 原语开启强制内联，未改变封装、数据布局或字节码。WSL ext4 上固定 vCPU 的 25 对 A/B 中，局部 `while` 从 38.08 ms 降至 16.73 ms（配对加速比 2.27，95% 自助法区间 2.21–2.32），Callgrind 指令数从 5.58 亿降至 3.49 亿。另一次同轮 25 次对照中，Candy 为 15.61 ms，Lua 5.4.7 为 12.92 ms，Candy 仍慢约 21%。额外内联无条件跳转未测出稳定收益，未保留。
