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
#include "src/common/candy_lib.h"
#include "src/struct/candy_wrap.h"
#include "src/method/candy_standard.h"

TEST(wrap, lifecycle){
  candy_wrap_t wrap = candy_wrap_none(0);
  wrap = candy_wrap_delete(wrap);
  EXPECT_EQ((uint64_t)wrap, (uint64_t)NULL);
}

TEST(wrap, string){
  candy_wrap_t wrap = candy_wrap_string(0, (candy_span_t){(void *)"hello world", sizeof("hello world")});
  EXPECT_STREQ((char *)candy_wrap_get_string(wrap).data, (char *)"hello world");
  EXPECT_EQ(candy_wrap_expand(wrap).size, sizeof("hello world"));
  wrap = candy_wrap_set_string(wrap, (candy_span_t){(void *)"bye world", sizeof("bye world")});
  EXPECT_STREQ((char *)candy_wrap_get_string(wrap).data, (char *)"bye world");
  EXPECT_EQ(candy_wrap_expand(wrap).size, sizeof("bye world"));
  wrap = (candy_wrap_t)candy_wrap_delete(wrap);
  EXPECT_EQ((uint64_t)wrap, (uint64_t)NULL);
}

TEST(wrap, integer){
  candy_wrap_t wrap = candy_wrap_integer(0, 114514);
  EXPECT_EQ(candy_wrap_get_integer(wrap), 114514);
  wrap = candy_wrap_set_integer(wrap, 1919810);
  EXPECT_EQ(candy_wrap_get_integer(wrap), 1919810);
  EXPECT_FLOAT_EQ(candy_wrap_get_float(wrap), 1919810.0f);
  wrap = (candy_wrap_t)candy_wrap_delete(wrap);
  EXPECT_EQ((uint64_t)wrap, (uint64_t)NULL);
}

TEST(wrap, float){
  candy_wrap_t wrap = candy_wrap_float(0, 6.626f);
  EXPECT_FLOAT_EQ(candy_wrap_get_float(wrap), 6.626f);
  EXPECT_EQ(candy_wrap_get_integer(wrap), 7);
  wrap = candy_wrap_set_float(wrap, 3.1415926f);
  EXPECT_FLOAT_EQ(candy_wrap_get_float(wrap), 3.1415926f);
  EXPECT_EQ(candy_wrap_get_integer(wrap), 3);
  wrap = (candy_wrap_t)candy_wrap_delete(wrap);
  EXPECT_EQ((uint64_t)wrap, (uint64_t)NULL);
}

TEST(wrap, cast){
  candy_wrap_t wrap = candy_wrap_none(0);
  wrap = candy_wrap_set_string(wrap, (candy_span_t){(void *)"hello world", sizeof("hello world")});
  EXPECT_STREQ((char *)candy_wrap_get_string(wrap).data, (char *)"hello world");
  wrap = candy_wrap_set_integer(wrap, 114514);
  EXPECT_EQ(candy_wrap_get_integer(wrap), 114514);
  wrap = candy_wrap_set_float(wrap, 3.1415926f);
  EXPECT_FLOAT_EQ(candy_wrap_get_float(wrap), 3.1415926f);
  wrap = candy_wrap_set_method(wrap, candy_std_print);
  EXPECT_EQ((uint64_t)candy_wrap_get_method(wrap), (uint64_t)candy_std_print);
  wrap = (candy_wrap_t)candy_wrap_delete(wrap);
  EXPECT_EQ((uint64_t)wrap, (uint64_t)NULL);
}
