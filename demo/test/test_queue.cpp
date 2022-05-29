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
#include "src/struct/candy_pack.h"
#include "src/struct/candy_queue.h"
#include "src/method/candy_standard.h"

extern "C" void candy_object_print(candy_object_t obj);

TEST(queue, pointer){
  candy_queue_t queue = candy_queue_create();
  EXPECT_EQ((uint64_t)candy_queue_pointer(queue, -2), (uint64_t)candy_get_next(queue));
  EXPECT_EQ((uint64_t)candy_queue_pointer(queue, -1), (uint64_t)candy_get_next(queue));
  EXPECT_EQ((uint64_t)candy_queue_pointer(queue, 0), (uint64_t)candy_get_next(queue));
  EXPECT_EQ((uint64_t)candy_queue_pointer(queue, 1), (uint64_t)candy_get_next(queue));
  EXPECT_EQ((uint64_t)candy_queue_pointer(queue, 2), (uint64_t)candy_get_next(queue));
  candy_enqueue(queue, -1, (candy_node_t)candy_pack_string(0, (char *)"hello world", sizeof("hello world")));
  candy_enqueue(queue, -1, (candy_node_t)candy_pack_integer(0, 114514));
  candy_enqueue(queue, -1, (candy_node_t)candy_pack_float(0, 3.1415926f));
  candy_object_print(queue);
  //candy_queue_pointer(queue, -4);/* error */
  EXPECT_EQ((uint64_t)candy_queue_pointer(queue, -3), (uint64_t)candy_get_next(queue));
  EXPECT_EQ((uint64_t)candy_queue_pointer(queue, -2), (uint64_t)candy_get_next(*candy_get_next(queue)));
  EXPECT_EQ((uint64_t)candy_queue_pointer(queue, -1), (uint64_t)candy_get_next(*candy_get_next(*candy_get_next(queue))));
  EXPECT_EQ((uint64_t)candy_queue_pointer(queue, 0), (uint64_t)candy_get_next(queue));
  EXPECT_EQ((uint64_t)candy_queue_pointer(queue, 1), (uint64_t)candy_get_next(*candy_get_next(queue)));
  EXPECT_EQ((uint64_t)candy_queue_pointer(queue, 2), (uint64_t)candy_get_next(*candy_get_next(*candy_get_next(queue))));
  EXPECT_EQ((uint64_t)candy_queue_pointer(queue, 3), (uint64_t)candy_get_next(*candy_get_next(*candy_get_next(*candy_get_next(queue)))));
  EXPECT_EQ((uint64_t)*candy_queue_pointer(queue, 3), (uint64_t)NULL);
  queue = candy_queue_delete(queue);
  EXPECT_EQ((uint64_t)queue, (uint64_t)NULL);
}

TEST(queue, clear){
  candy_queue_t queue = candy_queue_create();
  candy_enqueue(queue, -1, (candy_node_t)candy_pack_string(0, (char *)"hello world", sizeof("hello world")));
  candy_enqueue(queue, -1, (candy_node_t)candy_pack_integer(0, 114514));
  candy_enqueue(queue, -1, (candy_node_t)candy_pack_float(0, 3.1415926f));
  EXPECT_EQ(candy_queue_count(queue), 3);
  candy_queue_clear(queue);
  EXPECT_EQ(candy_queue_count(queue), 0);
  queue = candy_queue_delete(queue);
  EXPECT_EQ((uint64_t)queue, (uint64_t)NULL);
}

TEST(queue, fifo){
  uint16_t size = 0;
  candy_queue_t queue = candy_queue_create();
  candy_enqueue(queue, -1, (candy_node_t)candy_pack_string(0, (char *)"hello world", sizeof("hello world")));
  candy_enqueue(queue, -1, (candy_node_t)candy_pack_integer(0, 114514));
  candy_enqueue(queue, -1, (candy_node_t)candy_pack_float(0, 3.1415926f));
  candy_object_print(queue);
  EXPECT_STREQ(candy_pack_get_string((candy_pack_t)*candy_queue_pointer(queue, 0), &size), (char *)"hello world");
  EXPECT_EQ(candy_pack_get_integer((candy_pack_t)*candy_queue_pointer(queue, 1)), 114514);
  EXPECT_FLOAT_EQ(candy_pack_get_float((candy_pack_t)*candy_queue_pointer(queue, 2)), 3.1415926f);
  candy_dequeue(queue, 0);
  EXPECT_EQ(candy_pack_get_integer((candy_pack_t)*candy_queue_pointer(queue, 0)), 114514);
  EXPECT_FLOAT_EQ(candy_pack_get_float((candy_pack_t)*candy_queue_pointer(queue, 1)), 3.1415926f);
  candy_dequeue(queue, 0);
  EXPECT_FLOAT_EQ(candy_pack_get_float((candy_pack_t)*candy_queue_pointer(queue, 0)), 3.1415926f);
  candy_dequeue(queue, 0);
  EXPECT_EQ(candy_queue_empty(queue), !0);
  queue = candy_queue_delete(queue);
  EXPECT_EQ((uint64_t)queue, (uint64_t)NULL);
}

TEST(queue, lifo){
  uint16_t size = 0;
  candy_queue_t queue = candy_queue_create();
  candy_enqueue(queue, 0, (candy_node_t)candy_pack_string(0, (char *)"hello world", sizeof("hello world")));
  candy_enqueue(queue, 0, (candy_node_t)candy_pack_integer(0, 114514));
  candy_enqueue(queue, 0, (candy_node_t)candy_pack_float(0, 3.1415926f));
  candy_object_print(queue);
  EXPECT_STREQ(candy_pack_get_string((candy_pack_t)*candy_queue_pointer(queue, 2), &size), (char *)"hello world");
  EXPECT_EQ(candy_pack_get_integer((candy_pack_t)*candy_queue_pointer(queue, 1)), 114514);
  EXPECT_FLOAT_EQ(candy_pack_get_float((candy_pack_t)*candy_queue_pointer(queue, 0)), 3.1415926f);
  candy_dequeue(queue, 0);
  EXPECT_STREQ(candy_pack_get_string((candy_pack_t)*candy_queue_pointer(queue, 1), &size), (char *)"hello world");
  EXPECT_EQ(candy_pack_get_integer((candy_pack_t)*candy_queue_pointer(queue, 0)), 114514);
  candy_dequeue(queue, 0);
  EXPECT_STREQ(candy_pack_get_string((candy_pack_t)*candy_queue_pointer(queue, 0), &size), (char *)"hello world");
  candy_dequeue(queue, 0);
  EXPECT_EQ(candy_queue_empty(queue), !0);
  queue = candy_queue_delete(queue);
  EXPECT_EQ((uint64_t)queue, (uint64_t)NULL);
}

TEST(queue, cast){
  candy_queue_t queue = candy_queue_create();
  candy_enqueue(queue, -1, (candy_node_t)candy_pack_string(0, (char *)"hello world", sizeof("hello world")));
  candy_enqueue(queue, -1, (candy_node_t)candy_pack_integer(0, 114514));
  candy_enqueue(queue, -1, (candy_node_t)candy_pack_float(0, 3.1415926f));
  candy_pack_set_float((candy_pack_t)*candy_queue_pointer(queue, 1), 6.28f);
  EXPECT_EQ(candy_queue_count(queue), 3);
  EXPECT_FLOAT_EQ(candy_pack_get_float((candy_pack_t)*candy_queue_pointer(queue, 1)), 6.28f);
  queue = candy_queue_delete(queue);
  EXPECT_EQ((uint64_t)queue, (uint64_t)NULL);
}
