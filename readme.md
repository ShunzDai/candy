# Candy

## 简介

## VS Code 插件

仓库提供 [Candy Language 插件](./editors/vscode/README.md)，支持 `.candy` 语法高亮、
注释/缩进、代码片段和一键运行。已有 VSIX 时可直接安装，无需 npm：

```sh
cd editors/vscode
code --install-extension candy-language-0.1.1.vsix
```

从源码打包需要 Node.js 22+，运行脚本前需编译 Candy 并设置 `candy.executablePath`，
详见插件文档中的构建步骤和环境问题处理方法。

## 当前支持语法

### 类型

整数、浮点、布尔（`true`/`false`）、字符串（`"..."` 或 `'...'`）、`null`。

### 运算符

| 类别 | 运算符 |
|---|---|
| 算术 | `+` `-` `*` `/` `%` |
| 比较 | `<` `>` `<=` `>=` `==` `!=` |
| 位运算 | `&` `|` `^` `<<` `>>` |
| 逻辑 | `&&` `||` `!`（短路求值，`!` 为一元前缀） |

- 一元：`-`（负号）、`!`（逻辑非）。
- 算术与比较作用于整数/浮点；位运算作用于整数。
- 逻辑真值：仅 `false` 与 `null` 为假，其余为真。
- 复合赋值：`+=` `-=` `*=` `/=` `%=` `&=` `|=` `^=`，等价于 `a = a op b`（`a` 只求值一次）。

### 表与数组

- 数组字面量 `[1, 2, 3]`，下标从 0 开始。
- 表字面量：`{10, 20}`（整数键 0..n）或 `{"k": v}`（字符串键，JSON 风格）。
- 下标：`t[k]`（读）、`t[k] = v`（写）、`t[k] += v`（复合）。
- 全字面量的数组用一条 `SETLIST` 批量填充；混合/非常量数组退化为逐元素 `SETTABLE`。
- JSON 兼容：`null`、对象 `{"k": v}`、数组 `[...]` 可逐字书写。

### 控制流

```text
if (condition)
  ...
elif (other)
  ...
else
  ...
end

while (condition)
  ...
end

for (x : iterable)
  ...
end
```

- `if` 的 `elif` 可有多个、`else` 可省略，各分支共用一个 `end`。
- `for` 是 range-based（遍历数组），不是 C 三段式；数值循环请用 `while`。
- `break` 退出最近一层循环；`continue` 跳到下一次检查（对 `for` 即下一个元素）。二者不能跨越函数边界。

### 函数

```text
def name(a, b)
  ...
  return value
end
```

- 支持嵌套、递归、闭包（函数可作为值/返回值）。
- 顶层赋值与 `def` 绑定全局；函数内首次赋值绑定局部，之后更新局部槽；未绑定的名字读取回退全局。

### 内置

- `print(...)`：标量直接输出；表按缩进格式输出（连续整数键为数组 `[...]`，其余为对象 `{...}`，空表内联为 `[]`）。
  示例：`print({"a": {"b": "1"}, "c": 2})` 输出

   ```text
   {
     "a": {
       "b": "1"
     },
     "c": 2
   }
   ```
- `exit(...)`：退出进程。

## 源码分层

Candy 的模块按 13 层组织，只允许向下依赖（禁止环），见 [docs/architecture.md](docs/architecture.md)。
校验：`python3 tools/check_layers.py`。

## 参考文献

[1]李昂. PikaScript[EB/OL]. [link](https://gitee.com/Lyon1998/pikascript), 2022-03-09, 2022-03-09

[2]Roberto Ierusalimschy. lua[EB/OL]. [link](https://github.com/lua/lua), 2022-06-18, 2022-06-18

[3]骆国辉. 小试牛刀-递归下降算法（3）[EB/OL]. [link](https://mp.weixin.qq.com/s/4yv6JP8nVJCeUP8f_uvMFw), 2017-09-30, 2022-07-02

[4]猎户座. Python技法：实现简单的递归下降Parser[EB/OL]. [link](https://zhuanlan.zhihu.com/p/512300174), 2022-05-10, 2022-07-02

[5]Manistein. dummylua[EB/OL]. [link](https://github.com/Manistein/dummylua-tutorial), 2021-11-17, 2022-11-10

[6]游戏猫. Lua5.1.5源码分析之table实现[EB/OL]. [link](https://blog.csdn.net/gamekit/article/details/122975184), 2022-02-17, 2023-02-24

[7]co lin. 深入Lua：Table的实现[EB/OL]. [link](https://zhuanlan.zhihu.com/p/97830462), 2019-12-17, 2023-04-01

[8]frog-game. [Lua5.4.4源码].指令集[EB/OL]. [link](https://zhuanlan.zhihu.com/p/610168640), 2023-02-28, 2023-05-01

[9]科研者. 语法格式描述规范BNF、EBNF、ABNF[EB/OL]. [link](https://www.jianshu.com/p/15efcb0c06c8), 2017-09-22, 2023-05-04

[10]阿文. lua源码分析之语法分析 (干货)[EB/OL]. [link](https://zhuanlan.zhihu.com/p/581491614), 2022-11-11, 2024-02-13

[11] Xun. Lua篇 — 垃圾回收[EB/OL]. [link](https://fallingxun.github.io/post/lua/lua_gc/), 2023-11-07, 2024-12-29

[12] 码农成长宝典. Lua5.4源代码剖析[EB/OL]. [link](https://www.zhihu.com/column/c_1595011738562494465), 2024-05-22, 2025-02-09

[13] 云风. Lua参考手册5.3[EB/OL]. [link](https://wizardforcel.gitbooks.io/lua-doc/content/11.html)
