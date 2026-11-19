# 优化验证与测量约定

## 当前状态

默认 `src/` 为栈式 VM，当前构建为静态、non-PGO。当前图表使用 `final_stack.json` / `memory_stack.json`，时间采用端到端耗时，包含启动、解析、编译、执行与回收。扩展到 34 个时间用例与 12 个内存用例，分组和绑定差异见 [扩展基准](BENCHMARKS.md)。

旧报告中的数值保留作历史记录，但有以下限制：

- 部分旧 A/B 不是仅相差一个优化的源码版本，不能当作 CALLG 的因果收益。
- 分别运行的 Candy/Lua 比值变化包含机器负载与 Lua 自身波动；不能相除后当作某项优化的精确提升。
- 旧 PGO 训练多进程复用同一个 raw profile 文件，不足以证明训练负载被累计。应为每个进程生成唯一文件，再显式合并；不能据此断言布局是全部收益的来源。
- 旧 shell 忽略 `candy_dofile` 返回值；“退出 0”不等于成功。特别是旧文档引用的已删除 fib 脚本，其缺失没有被退出码捕获。现在 shell 对读取/执行/关闭失败返回非零。
- 旧 benchmark 用 `strip()` 比较输出、忽略 stderr，并且 warmup 仅运行偶数 pass。新版对 verify、warmup、timed run 统一校验完整输出及错误，实际执行指定次数的 warmup。
- 现有基准启动新进程，包含启动、解析、编译、执行和回收；扣除空脚本启动也不等于纯 VM 执行时间。当前尚未提供纯执行计时。

## 可重复 A/B

所有构建都放在 `build/` 内。两侧必须使用相同编译器、优化级别、LTO、链接类型和栈容量，PGO 显式关闭；记录唯一的预期差异。

```bash
python3 -m unittest discover -s bench -p 'test_*.py'
python3 bench/run_ab.py \
  --a build/variant-a/shell/candy \
  --b build/variant-b/shell/candy \
  --filter fib_34,calls_400k,globals_400k,arith_400k,compile_50k \
  --warmup 2 --reps 15 \
  --build-description '同编译器、O3/LTO、static、non-PGO、stack=128；仅切换待测优化' \
  --out build/variant-ab.json
```

脚本先检查正确性，再随机化用例顺序、交替执行 A/B。结果保留原始配对时间、可执行文件和源码 SHA-256、median ratio 及配对比值的 bootstrap 区间。区间只反映这些样本的波动，不涵盖系统性偏差，也不是架构性能保证。二进制 hash 不覆盖动态库；对共享构建必须另存库和构建信息。

不要覆盖历史 `final.json` 或更新图表，直到改动完成并通过验证。新负载需有独立预期值，且不参与指令布局调优。

## PGO 训练

如果另做 PGO 对照，必须让两种语言都明确列出是否启用 PGO；禁止把 PGO 收益归到源码修改。示意：

```bash
# 使用匹配当前编译器版本的 llvm-profdata。
LLVM_PROFILE_FILE='build/profiles/train-%p.profraw' build/instrumented/shell/candy bench/cases/calls_400k.candy
# 每个训练进程都有独立 raw profile；训练结束后统一合并。
"$(xcrun -f llvm-profdata)" merge -o build/profiles/train.profdata build/profiles/*.profraw
```

应先创建输出目录，profile 更新后 clean rebuild，避免旧 LTO 对象混用。profile 依赖源码和训练分布；CPU 型号并不是 profile 文件可用性的唯一条件。

## 独立语义验证

```bash
python3 bench/check_holdout.py --candy build/shell/candy
```

覆盖嵌套参数求值、局部值跨调用，以及参数求值重绑定全局函数时调用目标仍使用先前加载的值。
