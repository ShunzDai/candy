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
#include "src/struct/candy_queue.h"
#include "src/method/candy_standard.h"

TEST(queue, fifo){
  candy_queue_t queue = candy_queue_create();
  candy_enqueue(queue, candy_pack_none((char *)"none"), -1);
  candy_queue_print(queue);
  candy_enqueue(queue, candy_pack_string((char *)"string", (char *)"hello world", sizeof("hello world")), -1);
  candy_queue_print(queue);
  candy_enqueue(queue, candy_pack_integer((char *)"integer", 114514), -1);
  candy_queue_print(queue);
  candy_dequeue_bypos(queue, 0);
  candy_queue_print(queue);
  candy_dequeue_bypos(queue, 0);
  candy_queue_print(queue);
  candy_dequeue_bypos(queue, 0);
  candy_queue_print(queue);
  queue = candy_queue_delete(queue);
  EXPECT_EQ((uint64_t)queue, (uint64_t)NULL);
}

TEST(queue, lifo){
  candy_queue_t queue = candy_queue_create();
  candy_queue_print(queue);
  candy_enqueue(queue, candy_pack_none((char *)"none"), 0);
  candy_queue_print(queue);
  candy_enqueue(queue, candy_pack_string((char *)"string", (char *)"hello world", sizeof("hello world")), 0);
  candy_queue_print(queue);
  candy_enqueue(queue, candy_pack_integer((char *)"integer", 114514), 0);
  candy_queue_print(queue);
  candy_dequeue_bypos(queue, 0);
  candy_queue_print(queue);
  candy_dequeue_bypos(queue, 0);
  candy_queue_print(queue);
  candy_dequeue_bypos(queue, 0);
  candy_queue_print(queue);
  queue = candy_queue_delete(queue);
  EXPECT_EQ((uint64_t)queue, (uint64_t)NULL);
}

TEST(queue, clear){
  candy_queue_t queue = candy_queue_create();
  candy_enqueue(queue, candy_pack_none((char *)"none"), -1);
  candy_enqueue(queue, candy_pack_string((char *)"string", (char *)"hello world", sizeof("hello world")), -1);
  candy_enqueue(queue, candy_pack_integer((char *)"integer", 114514), -1);
  candy_enqueue(queue, candy_pack_float((char *)"float", 3.1415926f), -1);
  candy_enqueue(queue, candy_pack_method((char *)"mathod", candy_method_print), -1);
  queue = candy_queue_delete(queue);
  EXPECT_EQ((uint64_t)queue, (uint64_t)NULL);
}

TEST(queue, search){
  candy_queue_t queue = candy_queue_create();
  candy_enqueue(queue, candy_pack_none((char *)"none"), -1);
  candy_enqueue(queue, candy_pack_string((char *)"string", (char *)"hello world", sizeof("hello world")), -1);
  candy_enqueue(queue, candy_pack_integer((char *)"integer", 114514), -1);
  candy_enqueue(queue, candy_pack_float((char *)"float", 3.1415926f), -1);
  candy_enqueue(queue, candy_pack_method((char *)"method", candy_method_print), -1);
  candy_queue_print(queue);
  candy_dequeue_byname(queue, (char *)"integer");
  candy_queue_print(queue);
  candy_dequeue_byname(queue, (char *)"none");
  candy_queue_print(queue);
  queue = candy_queue_delete(queue);
  EXPECT_EQ((uint64_t)queue, (uint64_t)NULL);
}

TEST(queue, searchnull){
  candy_queue_t queue = candy_queue_create();
  candy_pack_t *pack = candy_queue_search(queue, (char *)"pack");
  EXPECT_EQ((uint64_t)pack, (uint64_t)NULL);
  queue = candy_queue_delete(queue);
  EXPECT_EQ((uint64_t)queue, (uint64_t)NULL);
}
