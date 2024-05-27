/**
  * Copyright 2022-2024 ShunzDai
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
#include "test.h"
#include "core/candy_array.h"
#include "core/candy_gc.h"

TEST(array, string) {
  candy_gc_t gc{};
  candy_gc_init(&gc, test_allocator, nullptr);
  candy_array_t *self = candy_array_create(&gc, CANDY_TYPE_CHAR, MASK_NONE);
  candy_array_append(self, &gc, (char *)"hello world", strlen("hello world"));
  EXPECT_EQ(candy_array_size(self), strlen("hello world"));
  EXPECT_MEMEQ(candy_array_data(self), (char *)"hello world", candy_array_size(self));
  candy_handler_t list[CANDY_TYPE_NUM];
  list[CANDY_TYPE_CHAR] = (candy_handler_t)candy_array_delete;
  candy_gc_deinit(&gc, list);
}

TEST(array, append) {
  candy_gc_t gc{};
  candy_gc_init(&gc, test_allocator, nullptr);
  candy_array_t *self = candy_array_create(&gc, CANDY_TYPE_CHAR, MASK_NONE);
  candy_array_append(self, &gc, (char *)"hello", strlen("hello"));
  EXPECT_EQ(candy_array_size(self), strlen("hello"));
  EXPECT_MEMEQ(candy_array_data(self), (char *)"hello world", candy_array_size(self));
  candy_array_append(self, &gc, (char *)" world", strlen(" world"));
  EXPECT_EQ(candy_array_size(self), strlen("hello world"));
  EXPECT_MEMEQ(candy_array_data(self), (char *)"hello world", candy_array_size(self));
  candy_handler_t list[CANDY_TYPE_NUM];
  list[CANDY_TYPE_CHAR] = (candy_handler_t)candy_array_delete;
  candy_gc_deinit(&gc, list);
}
