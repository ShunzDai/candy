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
#include "test_common.h"
#include "src/candy_lexer.h"
#include "mid_os.h"
#include <string>

#define TEST_LEXER(_name, _token, _exp, ...) TEST(lexer, _name) {tast_body(_token, _exp __VA_OPT__(,) __VA_ARGS__);}

using namespace std;

void try_func(void *ud) {
  (*(std::function<void()> *)ud)();
  delete (std::function<void()> *)ud;
}

template <typename ... supposed>
static void tast_body(candy_tokens_t token, const char exp[], supposed ... value) {
  int status = 0;
  candy_buffer_t *buffer = candy_buffer_create(CANDY_ATOMIC_BUFFER_SIZE, sizeof(char), true);
  candy_lexer_t lex;
  info_str info = {exp, (int)strlen(exp) + 1, 0};
  candy_lexer_init(&lex, buffer, _string_reader, &info);
  candy_wrap_t wrap;
  candy_wrap_init_none(&wrap);
  if ((status = candy_try_catch(buffer, try_func, new std::function<void()>([&lex, token, &wrap]() {
    EXPECT_EQ(candy_lexer_next(&lex, &wrap), token);
    EXPECT_EQ(candy_lexer_next(&lex, &wrap), CANDY_TK_NONE);
  }))) != 0)
    goto exit;
  if constexpr(sizeof...(value)) {
    auto val = std::get<0>(std::make_tuple(value ...));
    if constexpr (std::is_same<decltype(val), std::string_view>::value) {
      int size;
      char *str = candy_wrap_get_string(&wrap, &size);
      EXPECT_EQ(memcmp(str, val.data(), size), 0);
      EXPECT_EQ(size, val.size());
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
      EXPECT_EQ(*candy_wrap_get_integer(&wrap, NULL), val);
    }
    else if constexpr (
      std::is_same<decltype(val),  float>::value ||
      std::is_same<decltype(val), double>::value
    ) {
      EXPECT_PRED_FORMAT2(::testing::internal::CmpHelperFloatingPointEQ<candy_float_t>, *candy_wrap_get_float(&wrap, NULL), val);
    }
    else {
      assert(0);
    }
  }
  exit:
  candy_wrap_deinit(&wrap);
  candy_lexer_deinit(&lex);
  candy_buffer_delete(&buffer);
  EXPECT_EQ(status, 0);
}

TEST_LEXER(comment_0, CANDY_TK_NONE, "#")

TEST_LEXER(comment_1, CANDY_TK_NONE, "# hello world\r\n")

TEST_LEXER(comment_2, CANDY_TK_NONE,
  "# hello world\n"
  "# hi\n"
)

TEST_LEXER(string_0, CANDY_TK_STRING,
  "\"\"",
  ""sv
)

TEST_LEXER(string_1, CANDY_TK_STRING,
  "\"hello world\"",
  "hello world"sv
)

TEST_LEXER(string_2, CANDY_TK_STRING,
  "\"\\\\\"",/* "\\" */
  "\\"sv
)

TEST_LEXER(string_3, CANDY_TK_STRING,
  "\"\\\"\"",/* "\"" */
  "\""sv
)

TEST_LEXER(string_4, CANDY_TK_STRING,
  "\"\\hello world\"",/* "\hello world" */
  "\\hello world"sv
)

TEST_LEXER(string_hex, CANDY_TK_STRING,
  "\"\\x68\\x65\\x6C\\x6C\\x6F\\x20\\x77\\x6F\\x72\\x6C\\x64\"",/* "\x68\x65\x6C\x6C\x6F\x20\x77\x6F\x72\x6C\x64" */
  "hello world"sv
)

TEST_LEXER(string_oct, CANDY_TK_STRING,
  "\"\\150\\145\\154\\154\\157\\40\\167\\157\\162\\154\\144\\0\"",/* "\150\145\154\154\157\40\167\157\162\154\144\0" */
  "hello world\0"sv
)

TEST_LEXER(string_multiline_0, CANDY_TK_STRING,
  "''''''",
  ""sv
)

TEST_LEXER(string_multiline_1, CANDY_TK_STRING,
  "'''\n"
  "hello\n"
  "world\n"
  "'''",
  "\nhello\nworld\n"sv
)

TEST_LEXER(integer, CANDY_TK_INTEGER,       "114514",       114514)
TEST_LEXER(hex,     CANDY_TK_INTEGER,   "0xAbCd1234",   0xAbCd1234)
TEST_LEXER(float,   CANDY_TK_FLOAT  ,    "3.1415926",    3.1415926)
TEST_LEXER(sci_0,   CANDY_TK_FLOAT  , "0.31415926e1", 0.31415926e1)
TEST_LEXER(sci_1,   CANDY_TK_FLOAT  , "314.15926e-2", 314.15926e-2)

TEST_LEXER(vararg, CANDY_TK_VARARG, "...")

#define CANDY_KW_TEST
#include "src/candy_keyword.list"

TEST_LEXER(CANDY_TK_LPAREN , CANDY_TK_LPAREN ,  "(")
TEST_LEXER(CANDY_TK_RPAREN , CANDY_TK_RPAREN ,  ")")
TEST_LEXER(CANDY_TK_COMMA  , CANDY_TK_COMMA  ,  ",")
TEST_LEXER(CANDY_TK_DOT    , CANDY_TK_DOT    ,  ".")
TEST_LEXER(CANDY_TK_COLON  , CANDY_TK_COLON  ,  ":")
TEST_LEXER(CANDY_TK_LBRACE , CANDY_TK_LBRACE ,  "[")
TEST_LEXER(CANDY_TK_RBRACE , CANDY_TK_RBRACE ,  "]")
TEST_LEXER(CANDY_TK_BITAND , CANDY_TK_BITAND ,  "&")
TEST_LEXER(CANDY_TK_BITOR  , CANDY_TK_BITOR  ,  "|")
TEST_LEXER(CANDY_TK_BITNOT , CANDY_TK_BITNOT ,  "~")
TEST_LEXER(CANDY_TK_BITXOR , CANDY_TK_BITXOR ,  "^")
TEST_LEXER(CANDY_TK_MOD    , CANDY_TK_MOD    ,  "%")
TEST_LEXER(CANDY_TK_ADD    , CANDY_TK_ADD    ,  "+")
TEST_LEXER(CANDY_TK_SUB    , CANDY_TK_SUB    ,  "-")
TEST_LEXER(CANDY_TK_MUL    , CANDY_TK_MUL    ,  "*")
TEST_LEXER(CANDY_TK_DIV    , CANDY_TK_DIV    ,  "/")
TEST_LEXER(CANDY_TK_ASSIGN , CANDY_TK_ASSIGN ,  "=")
TEST_LEXER(CANDY_TK_GREATER, CANDY_TK_GREATER,  ">")
TEST_LEXER(CANDY_TK_LESS   , CANDY_TK_LESS   ,  "<")
TEST_LEXER(CANDY_TK_EXP    , CANDY_TK_EXP    , "**")
TEST_LEXER(CANDY_TK_FLRDIV , CANDY_TK_FLRDIV , "//")
TEST_LEXER(CANDY_TK_MODASS , CANDY_TK_MODASS , "%=")
TEST_LEXER(CANDY_TK_NEQUAL , CANDY_TK_NEQUAL , "!=")
TEST_LEXER(CANDY_TK_ADDASS , CANDY_TK_ADDASS , "+=")
TEST_LEXER(CANDY_TK_SUBASS , CANDY_TK_SUBASS , "-=")
TEST_LEXER(CANDY_TK_MULASS , CANDY_TK_MULASS , "*=")
TEST_LEXER(CANDY_TK_DIVASS , CANDY_TK_DIVASS , "/=")
TEST_LEXER(CANDY_TK_EQUAL  , CANDY_TK_EQUAL  , "==")
TEST_LEXER(CANDY_TK_GEQUAL , CANDY_TK_GEQUAL , ">=")
TEST_LEXER(CANDY_TK_LEQUAL , CANDY_TK_LEQUAL , "<=")
TEST_LEXER(CANDY_TK_RSHIFT , CANDY_TK_RSHIFT , ">>")
TEST_LEXER(CANDY_TK_LSHIFT , CANDY_TK_LSHIFT , "<<")

TEST(lexer, file_system) {
  FILE *f = fopen("../test/test_lexer.cdy", "r");
  fseek(f, 0, SEEK_END);
  int size = (int)ftell(f);
  fseek(f, 0, SEEK_SET);
  candy_buffer_t *buffer = candy_buffer_create(CANDY_ATOMIC_BUFFER_SIZE, sizeof(char), true);
  candy_lexer_t lex;
  info_file info = {f, size};
  candy_lexer_init(&lex, buffer, _file_reader, &info);
  candy_wrap_t wrap;
  candy_wrap_init_none(&wrap);
  EXPECT_EQ(candy_try_catch(buffer, try_func, new std::function<void()>([&lex, &wrap]() {
    EXPECT_EQ(candy_lexer_next(&lex, &wrap), CANDY_TK_def);
    EXPECT_EQ(candy_lexer_next(&lex, &wrap), CANDY_TK_IDENT);
    EXPECT_EQ(candy_lexer_next(&lex, &wrap), CANDY_TK_LPAREN);
    EXPECT_EQ(candy_lexer_next(&lex, &wrap), CANDY_TK_IDENT);
    EXPECT_EQ(candy_lexer_next(&lex, &wrap), CANDY_TK_RPAREN);
    EXPECT_EQ(candy_lexer_next(&lex, &wrap), CANDY_TK_COLON);
    EXPECT_EQ(candy_lexer_next(&lex, &wrap), CANDY_TK_IDENT);
    EXPECT_EQ(candy_lexer_next(&lex, &wrap), CANDY_TK_LPAREN);
    EXPECT_EQ(candy_lexer_next(&lex, &wrap), CANDY_TK_IDENT);
    EXPECT_EQ(candy_lexer_next(&lex, &wrap), '+');
    EXPECT_EQ(candy_lexer_next(&lex, &wrap), CANDY_TK_STRING);
    EXPECT_EQ(candy_lexer_next(&lex, &wrap), CANDY_TK_RPAREN);
    EXPECT_EQ(candy_lexer_next(&lex, &wrap), CANDY_TK_return);
    EXPECT_EQ(candy_lexer_next(&lex, &wrap), CANDY_TK_INTEGER);
    EXPECT_EQ(candy_lexer_next(&lex, &wrap), CANDY_TK_if);
    EXPECT_EQ(candy_lexer_next(&lex, &wrap), CANDY_TK_IDENT);
    EXPECT_EQ(candy_lexer_next(&lex, &wrap), dual_ope('=', '='));
    EXPECT_EQ(candy_lexer_next(&lex, &wrap), CANDY_TK_STRING);
    candy_wrap_deinit(&wrap);
    EXPECT_EQ(candy_lexer_next(&lex, &wrap), CANDY_TK_COLON);
    EXPECT_EQ(candy_lexer_next(&lex, &wrap), CANDY_TK_IDENT);
    EXPECT_EQ(candy_lexer_next(&lex, &wrap), CANDY_TK_LPAREN);
    EXPECT_EQ(candy_lexer_next(&lex, &wrap), CANDY_TK_STRING);
    EXPECT_EQ(candy_lexer_next(&lex, &wrap), CANDY_TK_RPAREN);
    EXPECT_EQ(candy_lexer_next(&lex, &wrap), CANDY_TK_NONE);
  })), 0);
  candy_lexer_deinit(&lex);
  candy_buffer_delete(&buffer);
  fclose(info.f);
}
