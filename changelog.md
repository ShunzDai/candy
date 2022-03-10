# Change log

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
