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
#include "src/candy_io.h"
#include "src/candy_lexer.h"
#include "src/candy_reader.h"
#include "mid_os.h"
#include <string>

#define TEST_LEXER(_name, _token, _exp, ...) TEST(lexer, _name) {tast_body(_token, _exp __VA_OPT__(,) __VA_ARGS__);}

using namespace std;

template <typename ... supposed>
static void tast_body(candy_tokens_t token, const char exp[], supposed ... value) {
  candy_io_t io;
  candy_lexer_t lex;
  str_info info = {exp, strlen(exp), 0};
  candy_io_init(&io);
  candy_lexer_init(&lex, &io, string_reader, &info);
  ASSERT_EQ(candy_io_try_catch(&io, (candy_try_catch_cb_t)+[](candy_lexer_t *self, void *ud) {
    EXPECT_EQ(candy_lexer_next(self), *(candy_tokens_t *)ud);
    EXPECT_EQ(candy_lexer_next(self), TK_NULL);
  }, &lex, &token), 0);
  if constexpr(sizeof...(value)) {
    auto val = std::get<0>(std::make_tuple(value ...));
    if constexpr (std::is_same<decltype(val), std::string_view>::value) {
      auto *str = candy_wrap_get_string(&lex.curr.wrap);
      EXPECT_EQ(lex.curr.wrap.size, val.size());
      EXPECT_EQ(memcmp(str, val.data(), val.size()), 0);
    }
    else if constexpr (
      std::is_same<decltype(val),   int8_t>::value ||
      std::is_same<decltype(val),  int16_t>::value ||
      std::is_same<decltype(val),  int32_t>::value ||
      std::is_same<decltype(val),  int64_t>::value ||
      std::is_same<decltype(val),  uint8_t>::value ||
      std::is_same<decltype(val), uint16_t>::value ||
      std::is_same<decltype(val), uint32_t>::value ||
      std::is_same<decltype(val), uint64_t>::value
    ) {
      EXPECT_EQ(*candy_wrap_get_integer(&lex.curr.wrap), val);
    }
    else if constexpr (
      std::is_same<decltype(val),  float>::value ||
      std::is_same<decltype(val), double>::value
    ) {
      EXPECT_PRED_FORMAT2(::testing::internal::CmpHelperFloatingPointEQ<candy_float_t>, *candy_wrap_get_float(&lex.curr.wrap), val);
    }
    else {
      assert(0);
    }
  }
  candy_lexer_deinit(&lex);
  candy_io_deinit(&io);
}

TEST_LEXER(comment_0, TK_NULL, "#")

TEST_LEXER(comment_1, TK_NULL, "# hello world\r\n")

TEST_LEXER(comment_2, TK_NULL,
  "# hello world\n"
  "# hi\n"
)

TEST_LEXER(string_0, TK_STRING,
  "\"\"",
  ""sv
)

TEST_LEXER(string_1, TK_STRING,
  "\"hello world\"",
  "hello world"sv
)

TEST_LEXER(string_2, TK_STRING,
  "\"\\\\\"",/* "\\" */
  "\\"sv
)

TEST_LEXER(string_3, TK_STRING,
  "\"\\\"\"",/* "\"" */
  "\""sv
)

TEST_LEXER(string_4, TK_STRING,
  "\"\\hello world\"",/* "\hello world" */
  "\\hello world"sv
)

TEST_LEXER(string_hex, TK_STRING,
  "\"\\x68\\x65\\x6C\\x6C\\x6F\\x20\\x77\\x6F\\x72\\x6C\\x64\"",/* "\x68\x65\x6C\x6C\x6F\x20\x77\x6F\x72\x6C\x64" */
  "hello world"sv
)

TEST_LEXER(string_oct, TK_STRING,
  "\"\\150\\145\\154\\154\\157\\40\\167\\157\\162\\154\\144\\0\"",/* "\150\145\154\154\157\40\167\157\162\154\144\0" */
  "hello world\0"sv
)

TEST_LEXER(string_multiline_0, TK_STRING,
  "''''''",
  ""sv
)

TEST_LEXER(string_multiline_1, TK_STRING,
  "'''\n"
  "hello\n"
  "world\n"
  "'''",
  "\nhello\nworld\n"sv
)

TEST_LEXER(integer, TK_INTEGER,       "114514",       114514)
TEST_LEXER(hex,     TK_INTEGER,   "0xAbCd1234",   0xAbCd1234)
TEST_LEXER(float,   TK_FLOAT  ,    "3.1415926",    3.1415926)
TEST_LEXER(sci_0,   TK_FLOAT  , "0.31415926e1", 0.31415926e1)
TEST_LEXER(sci_1,   TK_FLOAT  , "314.15926e-2", 314.15926e-2)

TEST_LEXER(vararg, TK_VARARG, "...")

#define CANDY_KW_TEST
#include "src/candy_keyword.list"

TEST_LEXER(TK_LPAREN , TK_LPAREN ,  "(")
TEST_LEXER(TK_RPAREN , TK_RPAREN ,  ")")
TEST_LEXER(TK_COMMA  , TK_COMMA  ,  ",")
TEST_LEXER(TK_DOT    , TK_DOT    ,  ".")
TEST_LEXER(TK_COLON  , TK_COLON  ,  ":")
TEST_LEXER(TK_LBRACE , TK_LBRACE ,  "[")
TEST_LEXER(TK_RBRACE , TK_RBRACE ,  "]")
TEST_LEXER(TK_BITAND , TK_BITAND ,  "&")
TEST_LEXER(TK_BITOR  , TK_BITOR  ,  "|")
TEST_LEXER(TK_BITNOT , TK_BITNOT ,  "~")
TEST_LEXER(TK_BITXOR , TK_BITXOR ,  "^")
TEST_LEXER(TK_MOD    , TK_MOD    ,  "%")
TEST_LEXER(TK_ADD    , TK_ADD    ,  "+")
TEST_LEXER(TK_SUB    , TK_SUB    ,  "-")
TEST_LEXER(TK_MUL    , TK_MUL    ,  "*")
TEST_LEXER(TK_DIV    , TK_DIV    ,  "/")
TEST_LEXER(TK_ASSIGN , TK_ASSIGN ,  "=")
TEST_LEXER(TK_GREATER, TK_GREATER,  ">")
TEST_LEXER(TK_LESS   , TK_LESS   ,  "<")
TEST_LEXER(TK_EXP    , TK_EXP    , "**")
TEST_LEXER(TK_FLRDIV , TK_FLRDIV , "//")
TEST_LEXER(TK_MODASS , TK_MODASS , "%=")
TEST_LEXER(TK_NEQUAL , TK_NEQUAL , "!=")
TEST_LEXER(TK_ADDASS , TK_ADDASS , "+=")
TEST_LEXER(TK_SUBASS , TK_SUBASS , "-=")
TEST_LEXER(TK_MULASS , TK_MULASS , "*=")
TEST_LEXER(TK_DIVASS , TK_DIVASS , "/=")
TEST_LEXER(TK_EQUAL  , TK_EQUAL  , "==")
TEST_LEXER(TK_GEQUAL , TK_GEQUAL , ">=")
TEST_LEXER(TK_LEQUAL , TK_LEQUAL , "<=")
TEST_LEXER(TK_RSHIFT , TK_RSHIFT , ">>")
TEST_LEXER(TK_LSHIFT , TK_LSHIFT , "<<")

TEST(lexer, file_system) {
  FILE *f = fopen("../test/test_lexer.cdy", "r");
  fseek(f, 0, SEEK_END);
  size_t size = ftell(f);
  fseek(f, 0, SEEK_SET);
  candy_io_t io;
  candy_lexer_t lex;
  file_info info = {f, size};
  candy_io_init(&io);
  candy_lexer_init(&lex, &io, file_reader, &info);
  ASSERT_EQ(candy_io_try_catch(&io, (candy_try_catch_cb_t)+[](candy_lexer_t *self, void *ud) {
    EXPECT_EQ(candy_lexer_next(self), TK_def);
    candy_wrap_deinit(&self->curr.wrap);
    EXPECT_EQ(candy_lexer_next(self), TK_IDENT);
    candy_wrap_deinit(&self->curr.wrap);
    EXPECT_EQ(candy_lexer_next(self), '(');
    candy_wrap_deinit(&self->curr.wrap);
    EXPECT_EQ(candy_lexer_next(self), TK_IDENT);
    candy_wrap_deinit(&self->curr.wrap);
    EXPECT_EQ(candy_lexer_next(self), ')');
    candy_wrap_deinit(&self->curr.wrap);
    EXPECT_EQ(candy_lexer_next(self), ':');
    candy_wrap_deinit(&self->curr.wrap);
    EXPECT_EQ(candy_lexer_next(self), TK_IDENT);
    candy_wrap_deinit(&self->curr.wrap);
    EXPECT_EQ(candy_lexer_next(self), '(');
    candy_wrap_deinit(&self->curr.wrap);
    EXPECT_EQ(candy_lexer_next(self), TK_IDENT);
    candy_wrap_deinit(&self->curr.wrap);
    EXPECT_EQ(candy_lexer_next(self), '+');
    candy_wrap_deinit(&self->curr.wrap);
    EXPECT_EQ(candy_lexer_next(self), TK_STRING);
    candy_wrap_deinit(&self->curr.wrap);
    EXPECT_EQ(candy_lexer_next(self), ')');
    candy_wrap_deinit(&self->curr.wrap);
    EXPECT_EQ(candy_lexer_next(self), TK_return);
    candy_wrap_deinit(&self->curr.wrap);
    EXPECT_EQ(candy_lexer_next(self), TK_INTEGER);
    candy_wrap_deinit(&self->curr.wrap);
    EXPECT_EQ(candy_lexer_next(self), TK_if);
    candy_wrap_deinit(&self->curr.wrap);
    EXPECT_EQ(candy_lexer_next(self), TK_IDENT);
    candy_wrap_deinit(&self->curr.wrap);
    EXPECT_EQ(candy_lexer_next(self), dual_ope('=', '='));
    candy_wrap_deinit(&self->curr.wrap);
    EXPECT_EQ(candy_lexer_next(self), TK_STRING);
    candy_wrap_deinit(&self->curr.wrap);
    EXPECT_EQ(candy_lexer_next(self), ':');
    candy_wrap_deinit(&self->curr.wrap);
    EXPECT_EQ(candy_lexer_next(self), TK_IDENT);
    candy_wrap_deinit(&self->curr.wrap);
    EXPECT_EQ(candy_lexer_next(self), '(');
    candy_wrap_deinit(&self->curr.wrap);
    EXPECT_EQ(candy_lexer_next(self), TK_STRING);
    candy_wrap_deinit(&self->curr.wrap);
    EXPECT_EQ(candy_lexer_next(self), ')');
    candy_wrap_deinit(&self->curr.wrap);
    EXPECT_EQ(candy_lexer_next(self), TK_NULL);
    candy_wrap_deinit(&self->curr.wrap);
  }, &lex, nullptr), 0);
  candy_lexer_deinit(&lex);
  candy_io_deinit(&io);
  fclose(info.f);
}
