/**
  * Copyright 2022-2023 ShunzDai
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
#include "src/candy_stack.h"

TEST(stack, init) {
  candy_stack_t *s = candy_stack_create();
  candy_stack_delete(&s);
  EXPECT_EQ((uint64_t)s, 0);
}

TEST(stack, push_pop) {
  candy_stack_t *s = candy_stack_create();
  candy_stack_push_integer(s, 0);
  candy_stack_push_integer(s, 1);
  candy_stack_push_integer(s, 2);
  candy_stack_push_integer(s, 3);
  candy_stack_push_integer(s, 4);
  candy_stack_push_integer(s, 5);
  candy_stack_push_integer(s, 6);
  candy_stack_push_integer(s, 7);
  candy_stack_push_integer(s, 8);
  EXPECT_EQ(candy_stack_pull_integer(s), 8);
  EXPECT_EQ(candy_stack_pull_integer(s), 7);
  EXPECT_EQ(candy_stack_pull_integer(s), 6);
  EXPECT_EQ(candy_stack_pull_integer(s), 5);
  EXPECT_EQ(candy_stack_pull_integer(s), 4);
  EXPECT_EQ(candy_stack_pull_integer(s), 3);
  EXPECT_EQ(candy_stack_pull_integer(s), 2);
  EXPECT_EQ(candy_stack_pull_integer(s), 1);
  EXPECT_EQ(candy_stack_pull_integer(s), 0);
  candy_stack_delete(&s);
  EXPECT_EQ((uint64_t)s, 0);
}
