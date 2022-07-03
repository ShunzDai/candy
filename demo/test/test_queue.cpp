/**
  * Copyright 2022 ShunzDai
  *
  * Licensed under the Apache License, Version 2.0 (the "License");
  * you may not use this file except in compliance with the License.
  * You may obtain a copy of the License at
  *
  *     http://www.apache.org/licenses/LICENSE-2.0
  *
  * Unless required by applicable law or agreed to in writing, software
  * distributed under the License is distributed on an "AS IS" BASIS,
  * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  * See the License for the specific language governing permissions and
  * limitations under the License.
  */
#include "gtest/gtest.h"
#include "src/struct/candy_wrap.h"
#include "src/struct/candy_queue.h"

extern "C" int candy_object_delete(candy_object_t *obj);

TEST(queue, pointer) {
  candy_queue_t queue = candy_queue_create();
  EXPECT_EQ((uint64_t)candy_queue_pointer(queue, -2), (uint64_t)&queue->head);
  EXPECT_EQ((uint64_t)candy_queue_pointer(queue, -1), (uint64_t)&queue->head);
  EXPECT_EQ((uint64_t)candy_queue_pointer(queue, 0), (uint64_t)&queue->head);
  EXPECT_EQ((uint64_t)candy_queue_pointer(queue, 1), (uint64_t)&queue->head);
  EXPECT_EQ((uint64_t)candy_queue_pointer(queue, 2), (uint64_t)&queue->head);
  candy_enqueue(queue, -1, (candy_node_t)candy_wrap_string(0, "hello world", sizeof("hello world")));
  candy_enqueue(queue, -1, (candy_node_t)candy_wrap_integer(0, 114514));
  candy_enqueue(queue, -1, (candy_node_t)candy_wrap_float(0, 3.1415926f));
  // candy_queue_pointer(queue, -4);/* error */
  EXPECT_EQ((uint64_t)candy_queue_pointer(queue, -3), (uint64_t)&queue->head);
  EXPECT_EQ((uint64_t)candy_queue_pointer(queue, -2), (uint64_t)&queue->head->next);
  EXPECT_EQ((uint64_t)candy_queue_pointer(queue, -1), (uint64_t)&queue->head->next->next);
  EXPECT_EQ((uint64_t)candy_queue_pointer(queue, 0), (uint64_t)&queue->head);
  EXPECT_EQ((uint64_t)candy_queue_pointer(queue, 1), (uint64_t)&queue->head->next);
  EXPECT_EQ((uint64_t)candy_queue_pointer(queue, 2), (uint64_t)&queue->head->next->next);
  EXPECT_EQ((uint64_t)candy_queue_pointer(queue, 3), (uint64_t)&queue->head->next->next->next);
  EXPECT_EQ((uint64_t)*candy_queue_pointer(queue, 3), (uint64_t)NULL);
  candy_queue_delete(&queue, (candy_destroy_t)candy_object_delete);
  EXPECT_EQ((uint64_t)queue, (uint64_t)NULL);
}

TEST(queue, clear) {
  candy_queue_t queue = candy_queue_create();
  candy_enqueue(queue, -1, (candy_node_t)candy_wrap_string(0, "hello world", sizeof("hello world")));
  candy_enqueue(queue, -1, (candy_node_t)candy_wrap_integer(0, 114514));
  candy_enqueue(queue, -1, (candy_node_t)candy_wrap_float(0, 3.1415926f));
  EXPECT_EQ(candy_queue_count(queue), 3);
  candy_queue_clear(queue, (candy_destroy_t)candy_object_delete);
  EXPECT_EQ(candy_queue_count(queue), 0);
  candy_queue_delete(&queue, (candy_destroy_t)candy_object_delete);
  EXPECT_EQ((uint64_t)queue, (uint64_t)NULL);
}

TEST(queue, fifo) {
  candy_queue_t queue = candy_queue_create();
  candy_enqueue(queue, -1, (candy_node_t)candy_wrap_string(0, "hello world", sizeof("hello world")));
  candy_enqueue(queue, -1, (candy_node_t)candy_wrap_integer(0, 114514));
  candy_enqueue(queue, -1, (candy_node_t)candy_wrap_float(0, 3.1415926f));
  EXPECT_STREQ((char *)candy_wrap_get_string((candy_wrap_t)*candy_queue_pointer(queue, 0))->data, (char *)"hello world");
  EXPECT_EQ(candy_wrap_get_integer((candy_wrap_t)*candy_queue_pointer(queue, 1)), 114514);
  EXPECT_FLOAT_EQ(candy_wrap_get_float((candy_wrap_t)*candy_queue_pointer(queue, 2)), 3.1415926f);
  candy_dequeue(queue, 0, (candy_destroy_t)candy_object_delete);
  EXPECT_EQ(candy_wrap_get_integer((candy_wrap_t)*candy_queue_pointer(queue, 0)), 114514);
  EXPECT_FLOAT_EQ(candy_wrap_get_float((candy_wrap_t)*candy_queue_pointer(queue, 1)), 3.1415926f);
  candy_dequeue(queue, 0, (candy_destroy_t)candy_object_delete);
  EXPECT_FLOAT_EQ(candy_wrap_get_float((candy_wrap_t)*candy_queue_pointer(queue, 0)), 3.1415926f);
  candy_dequeue(queue, 0, (candy_destroy_t)candy_object_delete);
  EXPECT_EQ(candy_queue_empty(queue), !0);
  candy_queue_delete(&queue, (candy_destroy_t)candy_object_delete);
  EXPECT_EQ((uint64_t)queue, (uint64_t)NULL);
}

TEST(queue, lifo) {
  candy_queue_t queue = candy_queue_create();
  candy_enqueue(queue, 0, (candy_node_t)candy_wrap_string(0, "hello world", sizeof("hello world")));
  candy_enqueue(queue, 0, (candy_node_t)candy_wrap_integer(0, 114514));
  candy_enqueue(queue, 0, (candy_node_t)candy_wrap_float(0, 3.1415926f));
  EXPECT_STREQ((char *)candy_wrap_get_string((candy_wrap_t)*candy_queue_pointer(queue, 2))->data, (char *)"hello world");
  EXPECT_EQ(candy_wrap_get_integer((candy_wrap_t)*candy_queue_pointer(queue, 1)), 114514);
  EXPECT_FLOAT_EQ(candy_wrap_get_float((candy_wrap_t)*candy_queue_pointer(queue, 0)), 3.1415926f);
  candy_dequeue(queue, 0, (candy_destroy_t)candy_object_delete);
  EXPECT_STREQ((char *)candy_wrap_get_string((candy_wrap_t)*candy_queue_pointer(queue, 1))->data, (char *)"hello world");
  EXPECT_EQ(candy_wrap_get_integer((candy_wrap_t)*candy_queue_pointer(queue, 0)), 114514);
  candy_dequeue(queue, 0, (candy_destroy_t)candy_object_delete);
  EXPECT_STREQ((char *)candy_wrap_get_string((candy_wrap_t)*candy_queue_pointer(queue, 0))->data, (char *)"hello world");
  candy_dequeue(queue, 0, (candy_destroy_t)candy_object_delete);
  EXPECT_EQ(candy_queue_empty(queue), !0);
  candy_queue_delete(&queue, (candy_destroy_t)candy_object_delete);
  EXPECT_EQ((uint64_t)queue, (uint64_t)NULL);
}

TEST(queue, cast) {
  candy_queue_t queue = candy_queue_create();
  candy_enqueue(queue, -1, (candy_node_t)candy_wrap_string(0, "hello world", sizeof("hello world")));
  candy_enqueue(queue, -1, (candy_node_t)candy_wrap_integer(0, 114514));
  candy_enqueue(queue, -1, (candy_node_t)candy_wrap_float(0, 3.1415926f));
  candy_wrap_set_float((candy_wrap_t *)candy_queue_pointer(queue, 1), 6.28f);
  EXPECT_EQ(candy_queue_count(queue), 3);
  EXPECT_FLOAT_EQ(candy_wrap_get_float((candy_wrap_t)*candy_queue_pointer(queue, 1)), 6.28f);
  candy_queue_delete(&queue, (candy_destroy_t)candy_object_delete);
  EXPECT_EQ((uint64_t)queue, (uint64_t)NULL);
}

void iterator_cb(candy_wrap_t *wrap, void *args) {
  struct temp{float arg1; int arg2;};
  candy_wrap_set_integer(wrap, ((temp *)args)->arg2);
}

TEST(queue, iterator) {
  candy_queue_t queue = candy_queue_create();
  candy_enqueue(queue, -1, (candy_node_t)candy_wrap_string(0, "hello world", sizeof("hello world")));
  candy_enqueue(queue, -1, (candy_node_t)candy_wrap_integer(0, 114514));
  candy_enqueue(queue, -1, (candy_node_t)candy_wrap_float(0, 3.1415926f));
  struct {float arg1; int arg2;} args = {3.1415926f, 114514};
  candy_queue_iterator(queue, (candy_iterator_t)iterator_cb, &args);
  EXPECT_EQ(candy_wrap_get_integer((candy_wrap_t)*candy_queue_pointer(queue, 0)), 114514);
  EXPECT_EQ(candy_wrap_get_integer((candy_wrap_t)*candy_queue_pointer(queue, 1)), 114514);
  EXPECT_EQ(candy_wrap_get_integer((candy_wrap_t)*candy_queue_pointer(queue, 2)), 114514);
  candy_queue_delete(&queue, (candy_destroy_t)candy_object_delete);
  EXPECT_EQ((uint64_t)queue, (uint64_t)NULL);
}
