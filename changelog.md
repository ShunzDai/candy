# Change log

## 2022-04-12 build1000.5

- 修复对含有转义字符\'和\"的字符串的错误解析
- 新增对更多转义字符的支持
- 增加新的解析器单元测试例

## 2022-03-15 build1000.4

- 重构队列，现在队列会存储节点数量信息
- 去除`pack`的`size`获取接口和`type`获取接口，替换为`node`的宏接口
- 新增`pack.recursive`递归析构测试用例
- 测试词法分析
- 重构`assert`

## 2022-03-10 build1000.3

- 新增`candy_node_t`类型作为所有线性数据结构的链表指针成员. 设计思路是不创建任何此类型的实例，仅将其作为强制类型转换的辅助结构. 此次增加的`candy_node_t`类型与build1000.2之前的`candy_node_t`类型无关
- 重构`candy_pack_t` `candy_queue_t`及它们的系列方法，现在它们都继承`candy_node_t`类型，所有继承`candy_node_t`的数据结构复用`candy_queue_t`的系列方法，通过自身的析构函数实现内存释放
- 重构`candy_queue_t`的测试用例
- 优化`candy_queue_pointer`方法
- 修复修改链表中的泛型容器类型会导致链表断裂的问题
- 新增`src/struct`的[readme](src/struct/readme.md)
- 新增一些空文件，尚未构造代码
- 其他微小的工作

## 2022-03-10 build1000.2

- 重构`candy_node_t`的部分方法，现在`candy_string_t`支持无符号字符串的增删改
- 将`candy_node_t`更名为`candy_pack_t`，系列方法也随之更名
- 实现FIFO与LIFO的复用
- googletest切换到release-1.11.0
- 更新测试用例
- 增加开源许可

## 2022-03-09 build1000.1

- 新增`candy_queue_t`系列方法，`candy_object_t`系列方法
- 重构`candy_node_t`系列方法
- 更新测试用例
- 移动time33 hash函数声明实现到`candy_lib.c/h`

## 2022-03-09 build1000

- 初始化工程
- 测试测试`node`系列方法
