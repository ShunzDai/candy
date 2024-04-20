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
#include "src/candy_gc.h"
#include "src/candy_array.h"
#include "src/candy_lexer.h"
#include "src/candy_reader.h"
#include <string>

#define TEST_BODY(_name, _token, _exp, ...) TEST(lexer, _name) { tast_body<_token>(_exp __VA_OPT__(,) __VA_ARGS__); }
#define TEST_ASSERT(_name, _exp, ...) TEST_BODY(_name, TK_EOS, _exp __VA_OPT__(,) __VA_ARGS__)
#define TEST_NORMAL(_name, _token, _exp, ...) TEST_BODY(_name, _token, _exp __VA_OPT__(,) __VA_ARGS__)

using namespace std;

template <typename supposed>
static void test_assert(const candy_array_t *err, const supposed &val) {
  (void)err;
  if constexpr (std::is_same<supposed, std::string_view>::value) {
    EXPECT_EQ(candy_array_size(err), val.size() + 1);
    EXPECT_MEMEQ(candy_array_data(err), val.data(), val.size() + 1);
  }
  else {
    assert(0);
  }
}

template <typename supposed>
static void test_normal(const candy_meta_t &meta, const supposed &val) {
  if constexpr (std::is_same<supposed, std::string_view>::value) {
    EXPECT_EQ(candy_array_size(meta.s), val.size());
    EXPECT_MEMEQ(candy_array_data(meta.s), val.data(), val.size());
  }
  else if constexpr (std::is_integral<supposed>::value) {
    EXPECT_EQ(meta.i, val);
  }
  else if constexpr (std::is_floating_point<supposed>::value) {
    EXPECT_PRED_FORMAT2(::testing::internal::CmpHelperFloatingPointEQ<candy_float_t>, meta.f, val);
  }
  else {
    static_assert(!std::is_same<supposed, supposed>::value);
  }
}

template <candy_tokens_t token, typename ... supposed>
static void tast_body(const char exp[], const supposed & ... value) {
  struct catch_info {
    candy_lexer ls{};
    candy_meta_t next{};
  };
  catch_info cinfo{};
  candy_gc_t gc{};
  str_info info{exp, strlen(exp), 0};
  candy_gc_init(&gc, test_allocator, nullptr);
  candy_lexer_init(&cinfo.ls, &gc, string_reader, &info);
  candy_array_t *err = candy_exce_try(&cinfo.ls.ctx, (candy_exce_cb_t)+[](catch_info *self) {
    EXPECT_EQ(candy_lexer_lookahead(&self->ls), token);
    if (token != TK_EOS)
      self->next = *candy_lexer_next(&self->ls);
    EXPECT_EQ(candy_lexer_lookahead(&self->ls), TK_EOS);
  }, &cinfo);
  candy_lexer_deinit(&cinfo.ls);
  if constexpr(sizeof...(value)) {
    if (err)
      test_assert(err, value ...);
    else
      test_normal(cinfo.next, value ...);
  }
  candy_handler_t list[TYPE_NUM];
  list[TYPE_CHAR] = (candy_handler_t)candy_array_delete;
  candy_gc_deinit(&gc, list);
}

TEST_ASSERT(invalid_0, "0x",    "lexical error: invalid hexadecimal number"sv)
TEST_ASSERT(invalid_1, "0b",    "lexical error: invalid binary number"sv)
TEST_ASSERT(invalid_2, "0b1e",  "lexical error: invalid float number"sv)
TEST_ASSERT(invalid_3, "0b1.2", "lexical error: invalid float number"sv)
TEST_ASSERT(invalid_4, "0x1x",  "lexical error: extra text after expected end of number"sv)
TEST_ASSERT(invalid_5, "0x1.4", "lexical error: invalid float number"sv)
TEST_ASSERT(invalid_6, "1..2",  "lexical error: malformed number"sv)

TEST_NORMAL(empty, TK_EOS, "")

TEST_NORMAL(comment_0, TK_EOS, "#")

TEST_NORMAL(comment_1, TK_EOS, "# hello world\r\n")

TEST_NORMAL(comment_2, TK_EOS,
  "# hello world\n"
  "# hi\n"
)

TEST_NORMAL(string_0, TK_STRING,
  "\"\"",
  ""sv
)

TEST_NORMAL(string_1, TK_STRING,
  "\"hello world\"",
  "hello world"sv
)

TEST_NORMAL(string_2, TK_STRING,
  "\"\\\\\"",/* "\\" */
  "\\"sv
)

TEST_NORMAL(string_3, TK_STRING,
  "\"\\\"\"",/* "\"" */
  "\""sv
)

TEST_NORMAL(string_4, TK_STRING,
  "\"\\\\hello world\"",/* "\\hello world" */
  "\\hello world"sv
)

TEST_NORMAL(string_hex, TK_STRING,
  "\"\\x68\\x65\\x6C\\x6C\\x6F\\x20\\x77\\x6F\\x72\\x6C\\x64\"",/* "\x68\x65\x6C\x6C\x6F\x20\x77\x6F\x72\x6C\x64" */
  "hello world"sv
)

TEST_NORMAL(string_oct, TK_STRING,
  "\"\\150\\145\\154\\154\\157\\40\\167\\157\\162\\154\\144\\0\"",/* "\150\145\154\154\157\40\167\157\162\154\144\0" */
  "hello world\0"sv
)

TEST_NORMAL(string_multiline_0, TK_STRING,
  "''''''",
  ""sv
)

TEST_NORMAL(string_multiline_1, TK_STRING,
  "'''\n"
  "hello\n"
  "world\n"
  "'''",
  "\nhello\nworld\n"sv
)

TEST_NORMAL(string_chinese, TK_STRING,
  "'你好世界'",
  "你好世界"sv
)

TEST_NORMAL(hex, TK_INTEGER,   "0xAbCd1234",   0xAbCd1234)
TEST_NORMAL(dec, TK_INTEGER,       "114514",       114514)
TEST_NORMAL(oct, TK_INTEGER,     "00114514",     00114514)
TEST_NORMAL(bin, TK_INTEGER,    "0b0101110",    0b0101110)
TEST_NORMAL(float, TK_FLOAT,    "3.1415926",    3.1415926)
TEST_NORMAL(sci_0, TK_FLOAT, "0.31415926e1", 0.31415926e1)
TEST_NORMAL(sci_1, TK_FLOAT, "314.15926e-2", 314.15926e-2)
TEST_NORMAL(sci_2, TK_FLOAT, "314.15926e+2", 314.15926e+2)

TEST_NORMAL(ident_0, TK_IDENT, "i")
TEST_NORMAL(ident_1, TK_IDENT, "ifif")

#define CANDY_OPR_TEST
#include "src/candy_operator.list"

#define CANDY_KW_TEST
#include "src/candy_keyword.list"
