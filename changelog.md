# Change log

## 2022-07-03 build1000.13

- `wrap`的析构函数入参新增存储对象的析构函数
- 移动`lexer`模块到单独的文件，在原`parser`模块中实现了基于递归下降四则运算的抽象语法树
- 新增esp32 demo
- 优化构建结构
- 更新参考文献
- 其他微小的工作

## 2022-06-18 build1000.12

- 移除`candy_span_t`，现在带有`candy_span_t`入参的函数恢复到build1000.8. 理由是传结构体本身给函数的行为涉及到平台、编译器实现等特性，降低了代码鲁棒性.
- 新增`candy_view_t`作为`candy_span_t`的替代，设计理念是仅将其作为观察者，不能通过其修改数据. 原本返回`candy_span_t`的函数现在返回`candy_view_t`.
- 增强`parser`单元测试的鲁棒性
- 修复`lex`解析多行字符串时引号偏移值错误的问题
- 重构所有析构函数，现在析构函数传入二重指针，在函数内部处理野指针
- 优化build1000.9 change log的表述
- 其他微小的工作

## 2022-06-09 build1000.11

- 优化`parser`，现在不再有多行注释，因为python仅有单行注释，多行注释应当理解为不赋给任何变量的字符串
- 优化`object`
- 修复changelog-build1000.10的两处字符错误
- 整理`queue`代码
- 其他微小的工作

## 2022-06-05 build1000.10

- 将`candy_pack_t`更名为`candy_wrap_t`, `candy_types_t`更名为`candy_wraps_t`, 系列方法也随之更名
- 移除`candy_wrap_t`的`object`方法, 现在`wrap`与`object`无关. 新增`wrap`的`meta`方法.
- 优化私有变量的获取方法
- 用`wrap`的`meta`方法重构了`object`，解决了`wrap`与`object`相互引用的问题

## 2022-05-23 build1000.9

- 新增candy_span_t类型, 现在candy_string_t由candy_span_t实现
- 修改c int类型状态量的含义, 引入c bool类型
- 重构lexer
- 重构assert函数的实现
- 重构queue::pointer方法
- 暂停除parser外的单元测试
- 其他微小的工作

## 2022-04-24 build1000.8

- 重构所有线性数据结构, 现在线性数据结构都只有一个`next`成员变量与末尾的空数组, 空数组用来指向私有或公有变量. 这样可以避免内存对齐, 做到进一步节省线性数据结构的占用空间.

## 2022-04-18 build1000.7

- 模块化单行/多行注释的偏移函数, 标识符/关键字的token函数, 十进制整数/小数的token函数
- 增加新的测试例

## 2022-04-17 build1000.6

- 模块化string类型的token函数
- 移动标准函数表的位置
- 优化assert机制

## 2022-04-12 build1000.5

- 修复对含有转义字符\'和\"的字符串的错误解析
- 新增对更多转义字符的支持
- 增加新的解析器单元测试例

## 2022-03-15 build1000.4

- 重构队列, 现在队列会存储节点数量信息
- 去除`pack`的`size`获取接口和`type`获取接口, 替换为`node`的宏接口
- 新增`pack.recursive`递归析构测试用例
- 测试词法分析
- 重构`assert`

## 2022-03-10 build1000.3

- 新增`candy_node_t`类型作为所有线性数据结构的链表指针成员. 设计思路是不创建任何此类型的实例, 仅将其作为强制类型转换的辅助结构. 此次增加的`candy_node_t`类型与build1000.2之前的`candy_node_t`类型无关
- 重构`candy_pack_t` `candy_queue_t`及它们的系列方法, 现在它们都继承`candy_node_t`类型, 所有继承`candy_node_t`的数据结构复用`candy_queue_t`的系列方法, 通过自身的析构函数实现内存释放
- 重构`candy_queue_t`的测试用例
- 优化`candy_queue_pointer`方法
- 修复修改链表中的泛型容器类型会导致链表断裂的问题
- 新增`src/struct`的[readme](src/struct/readme.md)
- 新增一些空文件, 尚未构造代码
- 其他微小的工作

## 2022-03-10 build1000.2

- 重构`candy_node_t`的部分方法, 现在`candy_string_t`支持无符号字符串的增删改
- 将`candy_node_t`更名为`candy_pack_t`, 系列方法也随之更名
- 实现FIFO与LIFO的复用
- googletest切换到release-1.11.0
- 更新测试用例
- 增加开源许可

## 2022-03-09 build1000.1

- 新增`candy_queue_t`系列方法, `candy_object_t`系列方法
- 重构`candy_node_t`系列方法
- 更新测试用例
- 移动time33 hash函数声明实现到`candy_lib.c/h`

## 2022-03-09 build1000

- 初始化工程
- 测试测试`node`系列方法
