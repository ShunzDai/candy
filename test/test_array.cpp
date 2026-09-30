/**
  * Copyright 2022-2026 ShunzDai
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

TEST(array, string) {
  candy_gc_t gc{};
  candy_gc_init(&gc, nullptr, (candy_handler_t)candy_array_handler, test_allocator, nullptr);
  candy_array_t *self = candy_array_create(&gc, nullptr, CANDY_TYPE_CHAR);
  candy_array_append(self, &gc, nullptr, "hello world", strlen("hello world"));
  EXPECT_EQ(candy_array_size(self), strlen("hello world"));
  EXPECT_MEMEQ(candy_array_data(self), "hello world", candy_array_size(self));
  candy_gc_deinit(&gc);
}

TEST(array, append) {
  candy_gc_t gc{};
  candy_gc_init(&gc, nullptr, (candy_handler_t)candy_array_handler, test_allocator, nullptr);
  candy_array_t *self = candy_array_create(&gc, nullptr, CANDY_TYPE_CHAR);
  candy_array_append(self, &gc, nullptr, "hello", strlen("hello"));
  EXPECT_EQ(candy_array_size(self), strlen("hello"));
  EXPECT_MEMEQ(candy_array_data(self), "hello", candy_array_size(self));
  candy_array_append(self, &gc, nullptr, " world", strlen(" world"));
  EXPECT_EQ(candy_array_size(self), strlen("hello world"));
  EXPECT_MEMEQ(candy_array_data(self), "hello world", candy_array_size(self));
  candy_gc_deinit(&gc);
}

TEST(array, reuse) {
  candy_gc_t gc{};
  candy_gc_init(&gc, nullptr, (candy_handler_t)candy_array_handler, test_allocator, nullptr);
  candy_array_t *a = candy_array_create_const(&gc, nullptr, CANDY_TYPE_CHAR, "hello", strlen("hello"));
  candy_array_t *b = candy_array_create_const(&gc, nullptr, CANDY_TYPE_CHAR, "hello", strlen("hello"));
  EXPECT_EQ(a, b);
  candy_gc_deinit(&gc);
}

TEST(array, growth_reserve_and_resize_preserve_elements) {
  candy_gc_t gc{};
  ASSERT_EQ(candy_gc_init(&gc, nullptr, (candy_handler_t)candy_array_handler,
                          test_allocator, nullptr), CANDY_OK);
  auto *self = candy_array_create(&gc, nullptr, CANDY_TYPE_INTEGER);
  ASSERT_NE(self, nullptr);
  EXPECT_EQ(candy_array_size(self), 0U);
  candy_integer_t expected[300];
  for (size_t index = 0; index < 300; ++index) {
    expected[index] = static_cast<candy_integer_t>(index) * 3 - 200;
    ASSERT_EQ(candy_array_append(self, &gc, nullptr, &expected[index], 1), CANDY_OK);
  }
  ASSERT_EQ(candy_array_size(self), 300U);
  EXPECT_GE(candy_array_capacity(self), 300U);
  EXPECT_MEMEQ(candy_array_data(self), expected, sizeof(expected));
  candy_array_reserve(self, &gc, nullptr, 600);
  EXPECT_GE(candy_array_capacity(self), 600U);
  EXPECT_EQ(candy_array_size(self), 300U);
  EXPECT_MEMEQ(candy_array_data(self), expected, sizeof(expected));
  candy_array_resize(self, &gc, nullptr, 10);
  ASSERT_EQ(candy_array_size(self), 10U);
  EXPECT_MEMEQ(candy_array_data(self), expected, 10 * sizeof(expected[0]));
  candy_array_resize(self, &gc, nullptr, 0);
  ASSERT_EQ(candy_array_append(self, &gc, nullptr, expected, 0), CANDY_OK);
  EXPECT_EQ(candy_array_size(self), 0U);
  ASSERT_EQ(candy_array_append(self, &gc, nullptr, expected, 3), CANDY_OK);
  ASSERT_EQ(candy_array_size(self), 3U);
  EXPECT_MEMEQ(candy_array_data(self), expected, 3 * sizeof(expected[0]));
  ASSERT_EQ(candy_gc_deinit(&gc), CANDY_OK);
  EXPECT_EQ(candy_memory_used(candy_gc_memory(&gc)), 0U);
}
