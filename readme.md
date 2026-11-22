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

### EBNF

引号内是终结符，`[ … ]` 表示可选，`{ … }` 表示重复零次或多次，`|` 表示二选一或多选一。下列产生式描述当前实现，作用域和运行时约束另述。

```ebnf
program        = { import | export | statement } ;
block          = { statement } ;

import         = "import", string, "as", identifier, [ ";" ] ;
export         = "export", function-def, [ ";" ] ;
statement      = function-def | class-def | if-stmt | while-stmt | for-stmt
               | "break" | "continue" | "return", expression | name-stmt ;
function-def   = "def", identifier, parameters, block, "end" ;
class-def      = "class", identifier, { function-def }, "end" ;
parameters     = "(", [ identifier, { ",", identifier } ], ")" ;

if-stmt        = "if", condition, block, { "elif", condition, block },
                 [ "else", block ], "end" ;
while-stmt     = "while", condition, block, "end" ;
for-stmt       = "for", "(", identifier, ":", expression, ")", block, "end" ;
condition      = "(", expression, ")" ;

name-stmt      = identifier, [ arguments | member, ( arguments | "=", expression )
                            | index, ( arguments, { suffix } | assignment )
                            | scope-member, ( arguments, { suffix } | "=", expression )
                            | assignment ] ;
assignment     = assign-op, expression ;
assign-op      = "=" | "+=" | "-=" | "*=" | "/=" | "%=" | "&=" | "|=" | "^=" ;

expression     = disjunction ;
disjunction    = conjunction, { "||", conjunction } ;
conjunction    = comparison, { "&&", comparison } ;
comparison     = bitwise, { ( "<" | "<=" | ">" | ">=" | "==" | "!=" ), bitwise } ;
bitwise        = sum, { ( "&" | "|" | "^" | "<<" | ">>" ), sum } ;
sum            = product, { ( "+" | "-" ), product } ;
product        = { postfix, mul-op }, ( postfix | unary-op, product ) ;
mul-op         = "*" | "/" | "%" ;
unary-op       = "-" | "!" ;
postfix        = primary, { suffix } ;
suffix         = index | member | scope-member | arguments ;
index          = "[", expression, "]" ;
member         = ".", identifier ;
scope-member   = "::", identifier ;
arguments      = "(", [ expression-list ], ")" ;
primary        = literal | identifier | "(", expression, ")" | array | table | lambda ;

array          = "[", [ expression-list ], "]" ;
table          = "{", [ expression-list | field, { ",", field } ], "}" ;
field          = string, ":", expression ;
expression-list = expression, { ",", expression } ;

lambda         = "def", [ captures ], parameters, block, "end" ;
captures       = "[", [ capture-list | capture-default, [ ",", capture-list ] ], "]" ;
capture-default = "=" | "&" ;
capture-list   = capture, { ",", capture } ;
capture        = [ "&" ], identifier ;
literal        = integer | float | string | "true" | "false" | "null" ;
```

主要词法单元：

```ebnf
identifier     = ( letter | "_" ), { letter | digit | "_" } ;
integer        = nonzero, { digit } | "0", { octal-digit }
               | ( "0x" | "0X" ), hex-digit, { hex-digit }
               | ( "0b" | "0B" ), binary-digit, { binary-digit } ;
float          = digits, ( ".", { digit }, [ exponent ] | exponent ) ;
exponent       = ( "e" | "E" ), [ "+" | "-" ], digits ;
digits         = digit, { digit } ;
letter         = ? ASCII 字母 A–Z 或 a–z ? ;
digit          = "0" | nonzero ;
nonzero        = ? 数字 1–9 ? ;
octal-digit    = ? 数字 0–7 ? ;
hex-digit      = digit | ? 十六进制字母 A–F 或 a–f ? ;
binary-digit   = "0" | "1" ;
string         = ? 单引号、双引号或三重引号字符串，见下文 ? ;
```

词法与限制：

- `identifier` 以字母或 `_` 开头，后接任意个字母、数字或 `_`，不能是保留字。`integer` 支持十进制、前导 `0` 的八进制、`0x`/`0X` 十六进制和 `0b`/`0B` 二进制；`float` 从数字开始，可带小数点或 `e`/`E` 指数。负号是运算符，不是字面量的一部分。
- `string` 用单引号或双引号；三重引号允许跨行。`#` 开始行注释。空白和换行不作为语句分隔符；分号只在上面标出的导入、导出位置可选。参数、元素和字段列表不支持尾随逗号。
- 二元运算符在同一优先级按左结合；位运算和移位共用一层。当前一元 `-` 和 `!` 的操作数可包含乘、除和取模，例如 `-a * b` 按 `-(a * b)` 解析。表字面量由首项决定是键值表还是序列表；序列表的首项若以字符串字面量开头且后面还有运算，用括号包住该表达式。
- 后缀链适用于表达式，例如 `make()(x)`；独立语句由标识符起始，成员赋值只支持一级下标或点号，点号赋值只支持 `=`。`break`、`continue` 只能出现在当前函数的循环内。
- Lambda 省略捕获列表等同于 `[]`，`[=]` 默认按值捕获，`[&]` 默认按引用捕获。类用 `ClassName(...)` 调用 `__init__`；实例的 `obj.method(...)` 自动传入实例，方法声明仍显式写第一个参数 `self`。

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

### 模块

在文件顶层使用 `import "lib/math.candy" as math`，通过 `math::add(2, 3)` 调用模块导出的函数。`::` 不传入接收者；`.` 用于对象成员，实例方法调用会传入 `self`。模块使用 `export def add(a, b) ... end` 显式导出函数；其他顶层绑定不会进入导出表。

相对路径以声明 `import` 的文件所在目录为基准；没有源文件路径的输入以当前工作目录为基准。依赖在编译期递归解析，全部编译成功后，模块顶层代码按依赖顺序在入口脚本之前初始化。同一运行时按整理后的路径缓存成功初始化的模块：合并多余分隔符并折叠 `.`、`..`，不解析符号链接，也不把相对路径转换成绝对路径。函数和控制流块内不能写 `import`。

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
