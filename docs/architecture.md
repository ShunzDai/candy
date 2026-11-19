# Candy 源码分层

`src/` 下每个模块 = 同名 `.c` + `.h`（如 `candy_table.c` / `candy_table.h`）。
规则：**模块只能依赖更低的层，禁止同层互相依赖，禁止任何环。**

由 `tools/check_layers.py` 自动校验：

```bash
python3 tools/check_layers.py
# layer check OK: 46 files, 23 modules, 12 layers
```

## 分层表

| 层 | 模块 | 职责 |
|---|---|---|
| L0 | `candy_log` `candy_reader` `candy_types` | 日志、输入回调、基础类型与枚举 |
| L1 | `candy_priv` | 内部公共定义：对象/值联合体、mask 枚举、`candy_assert`、类型大小表 |
| L2 | `candy_exception` `candy_lib` `candy_object` `candy_token` | 异常/长跳、字符与哈希工具、GC 对象头、token 枚举 |
| L3 | `candy_binary` `candy_memory` | 字节序转换、分配器抽象 |
| L4 | `candy_hashmap` `candy_vector` | 开放寻址哈希表底座、动态数组 |
| L5 | `candy_buffer` `candy_gc` | 词法缓冲区、GC（标记-清扫 + 字符串池） |
| L6 | `candy_array` `candy_closure` `candy_userdef` `candy_wrap` | 数组对象（字符串/对象指针）、闭包、用户数据、值容器 |
| L7 | `candy_lexer` `candy_table` | 词法分析、语言表 |
| L8 | `candy_proto` | 函数原型：指令、常量池、布局 |
| L9 | `candy_parser` `candy_vm` | 递归下降编译器、寄存器式字节码 VM |
| L10 | `candy_state` | 运行时状态：把 parser/VM/GC 串起来（协程、栈视图） |
| L11 | `candy` `candy_utils` | 对外 API、内置函数（`print`/`exit`） |

`shell/` 是应用层，位于库之上，不在校验范围内。

## 依赖方向示例

```
L11 candy ──> candy_state ──> candy_vm ──> candy_proto ──> candy_table
                                  │              │              │
                                  └──> candy_parser ──> candy_lexer
                                                              │
                    candy_wrap ──> candy_gc <── candy_array <─┘
                                       │
                                  candy_hashmap ──> candy_memory
```

## 为什么这样分

- **L0–L4 是底座**：不包含任何语言语义，只有内存、容器、异常、基础类型。
- **L5 `candy_gc` 依赖 `candy_hashmap`（单向）**：字符串池用开放寻址表。哈希表本身通过参数接收 `candy_memory_t *`，不反向依赖 GC。
- **L6 `candy_wrap` 不依赖 `candy_array`**：字符串的内容哈希与内容比较都通过 GC 的事件通道（`EVT_HASH` / `EVT_EQUAL`）向对象索取，值容器因此只依赖 `candy_gc`，和数组同层。
- **L9 parser 与 vm 同层且互不依赖**：两者只共享 `candy_proto`（指令与常量池），互不引用，因此可以各自演进。
- **L10 `candy_state` 是唯一的汇聚层**：只有它同时依赖 parser 和 vm。

## 历史上出现过的环（已修复）

| 环 | 原因 | 修复方式 |
|---|---|---|
| `candy_gc ↔ candy_hashmap` | hashmap 调用 `candy_gc_memory(gc)` 取分配器 | hashmap 改为接收 `candy_memory_t *` 参数，不再 include `candy_gc.h` |
| `candy_wrap → candy_array`（后已消除） | 值容器直接调用 `candy_array_hash` / `candy_array_data` 比较字符串 | 改为通过 GC 事件 `EVT_HASH` / `EVT_EQUAL` 向对象索取哈希与相等，`candy_wrap` 不再 include `candy_array.h` |
| `candy_priv` 被当作万能中转站 | 值结构体/访问器放在 `candy_priv.h` | 基础结构体留在 `candy_priv.h`，公开 API 留在各自头文件 |
