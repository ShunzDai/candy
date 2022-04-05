# API

## `candy_pack`方法

`candy_pack_t`提供了一系列泛型容器的增删改查API

### API `candy_pack_none`

```c
candy_pack_t candy_pack_none(char *name)
```

- 描述: 创建一个`none`类型的容器
- 输入: `name` 变量名
- 输出: `candy_pack_t` 容器句柄

### API `candy_pack_string`

```c
candy_pack_t candy_pack_string(char *name, candy_string_t str, uint16_t size)
```

- 描述: 创建一个`string`类型的泛型容器
- 输入: `name` 变量名
- 输入: `str` 字符串，支持输入`uint8_t`类型
- 输入: `size` 字符串长度
- 输出: `candy_pack_t` 泛型容器句柄

### API `candy_pack_integer`

```c
candy_pack_t candy_pack_integer(char *name, candy_integer_t value)
```

- 描述: 创建一个`integer`类型的泛型容器
- 输入: `name` 变量名
- 输入: `value` 整数
- 输出: `candy_pack_t` 泛型容器句柄

### API `candy_pack_float`

```c
candy_pack_t candy_pack_float(char *name, candy_float_t value)
```

- 描述: 创建一个`float`类型的泛型容器
- 输入: `name` 变量名
- 输入: `value` 浮点数
- 输出: `candy_pack_t` 泛型容器句柄

### API `candy_pack_method`

```c
candy_pack_t candy_pack_method(char *name, candy_method_t method)
```

- 描述: 创建一个`method`类型的泛型容器
- 输入: `name` 方法名
- 输入: `method` 方法函数指针
- 输出: `candy_pack_t` 泛型容器句柄

### API `candy_pack_object`

```c
candy_pack_t candy_pack_object(char *name, candy_object_t obj)
```

- 描述: 创建一个`object`类型的泛型容器
- 输入: `name` 对象名
- 输入: `method` 对象指针
- 输出: `candy_pack_t` 泛型容器句柄

### API `candy_pack_delete`

```c
candy_pack_t candy_pack_delete(candy_pack_t pack)
```

- 描述: 释放一个泛型容器
- 输入: `pack` 将要释放的泛型容器
- 输出: `candy_pack_t` 输入的泛型容器指向的下一个泛型容器，也可用于清空已释放的泛型容器句柄

## `candy_queue`方法

`candy_queue_t`提供了一系列队列的增删改查API，实现了先入先出(FIFO)与(LIFO)等规则的复用

### API `candy_queue_create`

```c
candy_queue_t candy_queue_create(void)
```

- 描述: 创建一个队列
- 输入: `void`
- 输出: `candy_queue_t` 队列容器句柄

### API `candy_queue_delete`

```c
candy_queue_t candy_queue_delete(candy_queue_t queue)
```

- 描述: 释放一个队列
- 输入: `queue` 将要释放的队列容器
- 输出: `candy_queue_t` 空指针，用于清空已释放的队列容器句柄

### API `candy_queue_pointer`

```c
candy_node_t *candy_queue_pointer(candy_queue_t queue, int32_t pos)
```

- 描述: 指向队列中的指定节点
- 输入: `queue` 目标队列
- 输入: `pos` 析构函数
- 输出: `candy_node_t *` 指向目标节点的指针. 其行为请参考测试用例[queue.pointer](demo\test\test_queue.cpp)

### API `candy_enqueue`

```c
int candy_enqueue(candy_queue_t queue, int32_t pos, candy_node_t node)
```

- 描述: 将一个泛型容器挂载到队列的指定位置
- 输入: `queue` 目标队列容器
- 输入: `pos` 目标位置. 当pos超过当前最大节点序号时，返回0；当pos为负时，节点容器将挂载到队列尾(如果当前队列为空，那么被挂载的节点容器将成为第0个节点)；否则节点容器将挂载到原队列第pos个节点前，成为新的第pos个节点，原第pos个节点将成为插入后的第pos+1个节点(如果原第pos个节点为`NULL`，那么被挂载的节点容器的下一节点为`NULL`).
- 输入: `node` 目标节点容器
- 输出: `int` 0为失败，1为成功

### API `candy_dequeue`

```c
int candy_dequeue(candy_queue_t queue, int32_t pos)
```

- 描述: 释放队列指定位置的泛型容器
- 输入: `queue` 目标队列容器
- 输入: `pos` 节点序号，从0开始计数. 当pos超过当前最大节点序号时，返回0；当pos为负时，将释放队列尾的泛型容器；否则释放第pos个泛型容器
- 输出: `int` 0为失败，1为成功
