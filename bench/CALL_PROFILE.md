# 20 万调用负载诊断

## 为什么端到端比较不能直接代表调用速度

`scripts/calls.candy` 模板把 `id(1)` 重复展开成 20 万行，而不是在一个小循环内重复执行预编译调用。源码约 1.2 MB。每次 shell 启动都包含读文件、词法分析、parser/codegen、执行和最终 GC。

另一个 `calls_local_200k` 中 Lua 使用 `local function id`，Candy 仍是全局 `id`。这不仅比较调用入口，也比较局部函数读取与全局表查找；不能把全部差距归因给 C 递归帧。

## 对照 Lua 的实现

Lua 5.4 系列的 [luaD_precall / moveresults](https://www.lua.org/source/5.4/ldo.c.html) 分开处理脚本/C 函数与零/一/多返回值；脚本参数在函数后连续摆放，常见返回路径只做必要搬运。Lua VM 还有直接从寄存器返回值的路径。官方网页当前为 5.4.9，基准安装版本为 5.4.7，这里用于实现思路对照，不宣称两版机器码相同。

Candy 已有 callee+参数的连续栈窗口和 SCLOS 快路径。此次尝试两项局部借鉴，不重写 VM：

1. 按调用者需求跳过零返回值的搬运：调用负载无明确改善，fib 约慢 4%，已回退。
2. 融合 `LOADL + RETURN` 为通用 `RETURNL`，直接从参数/局部位置返回：预编译执行约快 12%，但端到端调用基本不变，fib 重复实测约慢 5%；相邻 RETURN 布局复测仍回退，已回退。不是只对 id 函数匹配，但依然没有足够综合收益。

当前默认源码 **不包含这两项实验优化**。新增返回语义测试保留；无需为未证实收益增加 opcode、元数据或调用缓存。

## 分阶段测量

新增 `profile_calls.c` 使用现有内部 parser 和公开 state 调用接口，不修改库 API。先编译 chunk、在 VM 栈保留 closure 根，再重复执行，编译与执行使用同一 monotonic wall clock 分开计时。正常执行期 GC 检查保持开启，显式 full GC 单独计时。被测 chunk 仍会定义 id 并执行 200001 次调用；因此“预编译执行”包含少量函数定义与全局写入，不是裸 call 指令周期。

保存的基线/RETURNL 候选各 5 个交替进程，每进程 2 次预热、10 次计时：

| 阶段 | 基线 | RETURNL 候选（已回退） |
|---|---:|---:|
| 编译 1.2 MB chunk 中位数 | 20.836 ms | 21.340 ms |
| 预编译 chunk 执行中位数 | 2.4705 ms | 2.1805 ms |

这解释了为什么调用入口变快约 12%，完整负载却只改善约 0–2%：主要时间在编译，而非执行。

复现工具：

```bash
# 两侧必须匹配源码头文件、config、编译器和库构建参数；所有编译仅放 build/
python3 bench/profile_calls.py --core build/src/core/libcandy_core.a --binary build/profile_calls --build-only
python3 bench/profile_calls.py --binary build/profile_calls --calls 200000 --reps 10 --warmup 2 --samples 5
# loop 模式编译小段源码，执行循环+调用，包含循环与全局变量成本
python3 bench/profile_calls.py --binary build/profile_calls --mode loop --calls 200000 --reps 10 --warmup 2
```

分阶段原始结果为 `calls-phases.json`，端到端两次 RETURNL 测量为 `call-returnl-ab.json` 与 `call-returnl-repeat.json`。候选均已撤销，不能把它们作为当前代码性能数据。

## 下一步

先优化重复标识符/函数调用语句的 lexer、parser 和 codegen 成本，再在预编译执行负载上评估紧凑调用帧或直接局部返回。局部函数支持也可以消除 Lua local 对照中的查表差异，但属于语言语义扩展，应单独设计，不混入本轮调用优化。
