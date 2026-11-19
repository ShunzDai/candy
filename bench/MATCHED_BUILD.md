# 对齐构建参数后的 Candy / Lua 对照

当前 `final_stack.json`、`memory_stack.json` 与两张图使用本轮匹配构建，而不是 Homebrew 动态 Lua。

## 配置

| 项目 | Candy | Lua |
|---|---|---|
| 编译器 | `/usr/bin/clang`（AppleClang 21） | 同左 |
| 核心库 | static | 官方 Lua 5.4.7 `liblua.a` |
| 优化 | O3、NDEBUG、LTO | 同左 |
| 对齐 | `-falign-functions=32 -falign-loops=32` | 同左 |
| 裁剪 | `-Wl,-dead_strip` | 同左 |
| PGO | OFF、profile 为空 | 不使用 |
| 运行依赖 | libSystem | libSystem |

`otool -L` 确认双方仅依赖 libSystem，没有 libcandy/liblua 动态依赖。macOS 不能据此称为完全静态系统二进制；这里 static 指语言核心。

Lua 使用官方源代码 https://www.lua.org/ftp/lua-5.4.7.tar.gz ，SHA-256：`9fbf5e28ef86c69858f6d3d34eccc32e911c1a28b4120ff3e84aaa70cfbf1e30`。未修改 Lua 源码，保留 `LUA_COMPAT_5_3`，启用 `LUA_USE_MACOSX`，未启用 readline。C 标准和平台功能配置仍按各项目要求，不声称两个项目所有宏完全相同。

构建都位于 `build/`，没有改动默认源码或替换 Homebrew 安装：

```bash
cmake -S . -B build/matched-candy \
  -DCMAKE_C_COMPILER=/usr/bin/clang -DCMAKE_CXX_COMPILER=/usr/bin/clang++ \
  -DCMAKE_BUILD_TYPE=Release -DCANDY_STATIC=ON \
  -DCANDY_PGO_GENERATE=OFF -DCANDY_PGO_PROFILE=
cmake --build build/matched-candy -j4

# 下载并校验后解压到 build/lua-matched/lua-5.4.7
make -C build/lua-matched/lua-5.4.7/src -j4 all \
  'CC=/usr/bin/clang -std=gnu99' \
  'CFLAGS=-O3 -DNDEBUG -flto -falign-functions=32 -falign-loops=32 -Wall -Wextra -DLUA_COMPAT_5_3 -DLUA_USE_MACOSX' \
  'LDFLAGS=-flto -Wl,-dead_strip' 'LIBS=-lm'

python3 bench/run_bench.py --candy build/matched-candy/shell/candy \
  --lua build/lua-matched/lua-5.4.7/src/lua --warmup 2 --out final_stack.json
python3 bench/measure_mem.py --candy build/matched-candy/shell/candy \
  --lua build/lua-matched/lua-5.4.7/src/lua --reps 3 --warmup 1 --out memory_stack.json
python3 bench/make_charts.py
```

默认 `build/shell/candy` 不变；当前测量选用 `build/matched-candy/shell/candy`。基准仍是端到端、34 项时间/12 项 RSS，输出严格校验；热循环不是纯执行计时。

## 最新结论

比值为 Candy/Lua 端到端中位数；本表已包含局部原地加法融合（`x = x + c` / `x = x + y` → 单条 `ADDLC`/`ADDLL`）：

| 用例 | 比值 |
|---|---:|
| 40 万项算术表达式 | 0.67 |
| 编译 20 万行 | 0.98 |
| 定义 10 万函数 | 1.06 |
| 全局读写展开 | 1.15 |
| fib(34) | 1.40 |
| fib 别名调用 | 1.75 |
| 双方局部展开调用 | 1.32 |
| while 全局 | 1.80 |
| while 局部 | 1.58 |
| for 局部 | 1.70 |
| 热全局函数调用 | 1.71 |
| 热局部函数调用 | 1.72 |
| 热参数函数调用 | 1.74 |
| 分支链 | 1.08 |
| break/continue | 1.47 |
| 自尾递归 | 1.09 |

配对 A/B（融合版 / 无融合版，同参数 Release Candy）确认原地加法为净收益：局部 `while` 0.790×、`for` 0.869×、全局热调用 0.821×、分支链 0.845×、break/continue 0.860×；fib 1.016×、算术 0.990×、编译 1.008×，无回退。数据 `aug-fusion-ab.json`。

## 已回退：比较跳转融合（JLTC）

参照 Lua 5.4 的“比较测试 + 跳过下一条 JMP”模式，尝试把 `if/while/for` 里的 `local < const` 从 `LTLC`(压布尔)+`JMPF`(弹布尔+跳转) 改为 `JLTC`(直接比较、为真则跳过下一条 JMP)，消除布尔值的 push/pop。

配对 A/B（JLTC / LTLC+JMPF）基本无收益：局部 `while` 0.991×、`for` 0.994×、分支链 0.995×、fib 1.005×、break/continue 1.030×、热调用 1.039×。省掉一条指令没有转化为提速，说明当前瓶颈在 dispatch/内存访问而非条件指令数。已回退，数据 `jltc-ab.json`。

## 已回退：帧布局统一（去除 has_callee）

尝试让 CALLSELF 也像普通 CALL 一样压入 callee 槽，从而删除每条局部访问里的 `has_callee` 分支与额外寻址算术。配对 A/B（统一帧 / has_callee 版）结果分裂：

- 局部 `while` 0.917×、`for` 0.938×、分支链 0.951×，略有收益；
- 热调用反而更慢：全局 1.185×、局部 1.157×、参数 1.098×；
- fib(34) 因每层递归帧多占一个 callee 槽而 `local stack overflow`，递归深度回退。

净效果为负，已整体回退；当前仍是融合版（带 has_callee）。这说明本 VM 对 dispatch/帧布局敏感，结构上看似更简单的改动不一定更快，也不能以牺牲递归深度换取局部访问分支。

## 已回退：显式帧（非递归单一解释器循环）

参照 Lua 5.4 的 `luaV_execute`，把脚本调用从 C 递归改为非递归：`candy_vm_t` 持有一个帧数组，`_execute_proto` 变成单一循环，CALL/CALLSELF/RETURN/TAILCALL 在循环内通过 `_frame_push`/`_frame_return` 压弹帧，循环局部变量缓存当前帧的 `code/count/pc/constants`。168 测试全部通过，语义与 GC 正确。

配对 A/B（显式帧 / C 递归，同参数 Release Candy）结果分裂：

- 平坦调用与循环变快：`while_local` 0.860×、`for_local` 0.925×、`calls_hot_local` 0.920×、`calls_hot_parameter` 0.928×、`calls_nested_local` 0.937×、`break_continue` 0.907×、`calls_400k` 0.952×；
- 深层递归变慢：fib(34) 1.167×、fib 别名 1.098×、自尾递归 1.070×。即便给 CALLSELF 加“复用 proto、不保存/恢复 inst/constants”快路径，fib 仍 1.198×。

净效果是权衡而非净收益，已整体回退（当前仍是 C 递归 + 原地加法融合版）。数据 `frames-ab.json` / `frames2-ab.json`。

结论：对栈式指令集而言，显式帧把“每条指令的栈 push/pop + 帧压弹”留在了循环里，平坦调用省掉了 C 递归层（收益），但 CALLSELF 密集的递归反而为每次压弹帧多付成本（回退）。要同时赢得调用与递归，需要把指令改成寄存器式（像 Lua 那样局部变量驻留寄存器，递归不再 push/pop），这是下一步的大改动方向，而非继续在栈式上打补丁。

此前 stock 动态 Lua 的结论不可继续当作同参数优势；本轮支持的结论是：Candy 的长算术和编译仍有优势，循环和热调用仍明显慢于匹配构建的 Lua。原地加法消除了局部累加的三条指令，但调用、循环条件回跳和帧进出仍是差距来源。内存优势大体保留，但 10 万函数约 44.1 MiB vs 42.8 MiB，仍略高。不能仅由本轮与历史差值分离 static、O3、LTO 各自贡献。

## 寄存器式 VM（最终）

按上文结论实现了完整的寄存器式字节码 + 非递归显式帧。指令集 ADD/ADDK/SUB/SUBK/LT/LTK/MOVE/LOADK/LOADG/SETG/CALL/RETURN/TAILCALL/JMP/JMPF；算术/比较的“寄存器 op 常量”用独立的 ADDK/SUBK/LTK（iabc，常量索引 9 位），无 RK 分支解码。局部变量号 = 声明时的 freereg，临时在语句间回退到 local_hwm，二者共享 freereg 不冲突（无预扫描）。proto 增加 nparams/nreg；栈容量 128→256（寄存器式帧含临时）。168 测试全通过，语义正确。

对称性与封装：

- `candy_vm_call` 恢复统一的 `_vmll_init → _call → _vmll_deinit`，`_call` 按 fn 类型对称 dispatch（CFUNC → _cfunc_call，SCLOS → _sclosure_entry 压帧 + _execute）。
- 全局表 `candy_table_get/set` 保持封装（结构仍在 .c），仅加 `CANDY_ALWAYS_INLINE`（`__attribute__((always_inline))`）让 LTO 跨 TU 内联，不暴露内部结构。
- JMPF 用 `CANDY_LIKELY(cond) return;` 生成分支（tbz）而非 csel，修复了 branch/break 的退化。

配对 A/B（寄存器式 / 栈式融合版，同参数 Release，`register-v-stack.json`）：全用例 A/B ≤ 1.04，即寄存器式全面不输栈式。关键：calls_hot_parameter 0.532×、calls_hot_local 0.605×、calls_hot_global 0.701×、for_local 0.817×、break_continue 0.829×、mutual 0.862×、fib_alias 0.870×、while_local 0.899×、fib_34 0.930×、branch_chain 0.967×、while_global 0.982×；仅 globals 1.024–1.037×（全局读写略慢）与 arith 大循环中性。

端到端 vs Lua（净比值）：arith 0.66（已更快）、branch_chain 1.07、calls_hot_local 1.17、globals 1.18、calls_hot_global 1.39、fib 1.39、while_local 1.62、for_local 1.88、while_global 2.04。即：调用从 1.7× 压到 1.1–1.2×、分支从 1.08× 压到 1.03×，但纯循环/全局循环仍慢于 Lua（循环体 5 条 vs Lua 4 条——Lua 的 LTI 把比较与跳转合一，Candy 的 LTK+JMPF 是两条；全局循环受 candy_hashmap 线性探测影响）。要继续追平 Lua，方向是“比较+跳转合一”指令与全局表字符串键的快速路径，且不得破坏封装。

## 寄存器式 VM · 最终图表数据

在对称入口、封装内联、JMPF 分支、vmbreak 免边界检查、callinfo/frame 瘦身、vmll 缓存之后，又做了两件事：

- `_frame_push`/`_frame_return` 去掉冗余的 `vm->ci = &self->ci`（`_vmll_init` 后地址不变）。
- **for 循环编译成与 while 相同的 5 条结构**：跳过 step 先编译 body，再用 lexer mark/rewind 把 step 发到 body 之后（`continue` 改走链表、`_loop_close` 统一 patch）。`for_local` 从 1.17× 降到 0.85×。

完整 vs Lua 端到端（中位数，不扣启动）：

- 更快：arith 0.65–0.75、while_local 0.80（净 0.71）、for_local 0.85（净 0.71）、branch_chain 0.81、break_continue 0.86、calls_hot 0.91–0.94、fib_global_32 0.89、compile 0.97–1.01、tail_self 0.93、nested_functions 0.93；
- 接近（≤1.1）：fib 1.00–1.03、fib_alias 1.03、globals 1.06–1.09、closures 1.09、mutual 1.09、while_global 1.09、calls_100k 1.11；
- 仍略慢（1.1–1.3）：calls_200k 1.13、calls_400k 1.14、calls_nested_local 1.22、calls_local_200k 1.27。

结论：算术、所有循环（while/for/branch/break）、热调用、深度递归、编译已快于或持平 Lua；fib 压到 ~1.00；剩余差距只剩普通函数调用（calls 1.1–1.3，帧压弹的固有开销，实测“重新推导”无收益）。图表 `bench/charts/candy_vs_lua_time.png` / `candy_vs_lua_memory.png` 已重建。

## 值栈与帧栈动态化

参考 Lua（`luaD_growstack` 按需翻倍 + CallInfo 按需），把固定的 `CANDY_CONFIG_VM_STACK_SIZE 256` 改为 `CANDY_CONFIG_VM_STACK_MIN 32` / `CANDY_CONFIG_VM_STACK_MAX 65536`。`s`（值栈）和 `frames`（帧栈）初始各 32 个元素，`_frame_push`/`_sclosure_entry`/`_vm_push` 在容量不足时用 `_vector_grow` 翻倍扩容。关键：`candy_callinfo` 的 `bos/tos` 本来就是相对 `stack[0]` 的偏移（不是绝对指针），所以 realloc 值栈后偏移不变，只需更新 `vmll.stack` 缓存——这和 Lua 用 `StkIdRel`（相对偏移）+ `correctstack` 是同一思路。

收益：每个 state 的帧栈从固定 10KB 降到初始 1.25KB，浅调用几乎不额外分配；深度递归（实测 f(1000)）按需增长。168 测试 + ASan 全通过，语义不变。

## 统一栈帧（链表）

把 `candy_callinfo` 和 `candy_frame` 合并成一个结构——参考 Lua 的 `CallInfo`：`prev/next`（双向链表）+ `bos/tos` + `pc`（返回地址）+ `inst/constants`。`candy_vm` 只保留 `base`（内联的第 0 帧）和 `ci`（当前帧指针），`frames` 数组和 `depth` 都删掉了。压帧 = `_next_ci`（复用 `ci->next` 或按需 `candy_gc_alloc`）+ 设字段 + `ci` 后移；弹帧 = `ci = ci->prev`。

消除了 `candy_callinfo.bos/tos` 与 `candy_frame.bos/tos` 的重复，弹帧从"恢复 5 个字段"变成"指针后移"。C 函数调用用栈上临时的 `candy_callinfo`（不污染链表帧），尾调用复用帧时同步 `inst/constants` 持久字段。168 测试 + ASan 全通过，A/B 无退化（fib 0.66×、calls 0.91–0.93×、while_local 0.50×）。
