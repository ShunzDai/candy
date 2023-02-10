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
#include "src/candy_wrap.h"

TEST(wrap, sstring) {
  candy_wrap_t wrap;
  candy_wrap_init_string(&wrap, (char *)"hello", sizeof("hello"));
  EXPECT_EQ(wrap.size, sizeof("hello"));
  EXPECT_STREQ(candy_wrap_get_string(&wrap, NULL), (char *)"hello");
  candy_wrap_deinit(&wrap);
}

TEST(wrap, lstring) {
  candy_wrap_t wrap;
  candy_wrap_init_string(&wrap, (char *)"hello world", sizeof("hello world"));
  EXPECT_EQ(wrap.size, sizeof("hello world"));
  EXPECT_STREQ(candy_wrap_get_string(&wrap, NULL), (char *)"hello world");
  candy_wrap_deinit(&wrap);
}
