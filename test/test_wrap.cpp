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
#include "test_common.h"
#include "src/candy_wrap.h"

TEST(wrap, scopy) {
  candy_wrap_t src{}, dst{};
  candy_wrap_set_string(&src, (char *)"hello", sizeof("hello"));
  candy_wrap_copy(&dst, &src);
  EXPECT_EQ(src.size, sizeof("hello"));
  EXPECT_MEMEQ(candy_wrap_get_string(&src), (char *)"hello", src.size);
  EXPECT_EQ(dst.size, sizeof("hello"));
  EXPECT_MEMEQ(candy_wrap_get_string(&dst), (char *)"hello", dst.size);
  candy_wrap_deinit(&src);
  candy_wrap_deinit(&dst);
}

TEST(wrap, lcopy) {
  candy_wrap_t src{}, dst{};
  candy_wrap_set_string(&src, (char *)"hello world", sizeof("hello world"));
  candy_wrap_copy(&dst, &src);
  EXPECT_EQ(src.size, sizeof("hello world"));
  EXPECT_MEMEQ(candy_wrap_get_string(&src), (char *)"hello world", src.size);
  EXPECT_EQ(dst.size, sizeof("hello world"));
  EXPECT_MEMEQ(candy_wrap_get_string(&dst), (char *)"hello world", dst.size);
  candy_wrap_deinit(&src);
  candy_wrap_deinit(&dst);
}

TEST(wrap, sstring) {
  candy_wrap_t wrap{};
  candy_wrap_set_string(&wrap, (char *)"hello", sizeof("hello"));
  EXPECT_EQ(wrap.size, sizeof("hello"));
  EXPECT_MEMEQ(candy_wrap_get_string(&wrap), (char *)"hello", wrap.size);
  candy_wrap_deinit(&wrap);
}

TEST(wrap, lstring) {
  candy_wrap_t wrap{};
  candy_wrap_set_string(&wrap, (char *)"hello world", sizeof("hello world"));
  EXPECT_EQ(wrap.size, sizeof("hello world"));
  EXPECT_MEMEQ(candy_wrap_get_string(&wrap), (char *)"hello world", wrap.size);
  candy_wrap_deinit(&wrap);
}

TEST(wrap, append_string_s2s) {
  candy_wrap_t wrap{};
  candy_wrap_set_string(&wrap, (char *)"he", 2);
  EXPECT_EQ(wrap.size, 2);
  EXPECT_MEMEQ(candy_wrap_get_string(&wrap), (char *)"he", wrap.size);
  candy_wrap_append(&wrap, (char *)"llo", 3);
  EXPECT_EQ(wrap.size, 5);
  EXPECT_MEMEQ(candy_wrap_get_string(&wrap), (char *)"hello", wrap.size);
  candy_wrap_deinit(&wrap);
}

TEST(wrap, append_string_s2l) {
  candy_wrap_t wrap{};
  candy_wrap_set_string(&wrap, (char *)"hello", 5);
  EXPECT_EQ(wrap.size, 5);
  EXPECT_MEMEQ(candy_wrap_get_string(&wrap), (char *)"hello", wrap.size);
  candy_wrap_append(&wrap, (char *)" world", 6);
  EXPECT_EQ(wrap.size, 11);
  EXPECT_MEMEQ(candy_wrap_get_string(&wrap), (char *)"hello world", wrap.size);
  candy_wrap_deinit(&wrap);
}

TEST(wrap, append_string_l2l_with_expand) {
  candy_wrap_t wrap{};
  candy_wrap_set_string(&wrap, (char *)"wubba lubba dub", 15);
  EXPECT_EQ(wrap.size, 15);
  EXPECT_MEMEQ(candy_wrap_get_string(&wrap), (char *)"wubba lubba dub", wrap.size);
  candy_wrap_append(&wrap, (char *)" dub", 4);
  EXPECT_EQ(wrap.size, 19);
  EXPECT_MEMEQ(candy_wrap_get_string(&wrap), (char *)"wubba lubba dub dub", wrap.size);
  candy_wrap_deinit(&wrap);
}

TEST(wrap, append_string_l2l_without_expand) {
  candy_wrap_t wrap{};
  candy_wrap_set_string(&wrap, (char *)"wubba lubba", 11);
  EXPECT_EQ(wrap.size, 11);
  EXPECT_MEMEQ(candy_wrap_get_string(&wrap), (char *)"wubba lubba", wrap.size);
  candy_wrap_append(&wrap, (char *)" dub", 4);
  EXPECT_EQ(wrap.size, 15);
  EXPECT_MEMEQ(candy_wrap_get_string(&wrap), (char *)"wubba lubba dub", wrap.size);
  candy_wrap_deinit(&wrap);
}
