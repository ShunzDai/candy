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

TEST(queue, enqueue){
  candy_queue_t queue = candy_queue_create();
  candy_enqueue(queue, candy_create_none((char *)"node0"));
  candy_queue_print(queue);
  candy_enqueue(queue, candy_create_none((char *)"node1"));
  candy_queue_print(queue);
  candy_enqueue(queue, candy_create_none((char *)"node2"));
  candy_queue_print(queue);
  candy_dequeue_bypos(queue, 1);
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
  candy_enqueue(queue, candy_create_none((char *)"node0"));
  candy_enqueue(queue, candy_create_none((char *)"node1"));
  candy_enqueue(queue, candy_create_none((char *)"node2"));
  candy_enqueue(queue, candy_create_none((char *)"node3"));
  candy_enqueue(queue, candy_create_none((char *)"node4"));
  queue = candy_queue_delete(queue);
  EXPECT_EQ((uint64_t)queue, (uint64_t)NULL);
}

TEST(queue, search){
  candy_queue_t queue = candy_queue_create();
  candy_enqueue(queue, candy_create_none((char *)"node0"));
  candy_enqueue(queue, candy_create_none((char *)"node1"));
  candy_enqueue(queue, candy_create_none((char *)"node2"));
  candy_enqueue(queue, candy_create_none((char *)"node3"));
  candy_enqueue(queue, candy_create_none((char *)"node4"));
  candy_queue_print(queue);
  candy_dequeue_byname(queue, (char *)"node1");
  candy_dequeue_byname(queue, (char *)"node3");
  candy_queue_print(queue);
  queue = candy_queue_delete(queue);
  EXPECT_EQ((uint64_t)queue, (uint64_t)NULL);
}
