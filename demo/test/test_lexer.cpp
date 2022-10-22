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
#include "src/core/candy_lexer.h"
#include <string>

#define TEST_LEXER(_name, _token, _code, line, column, ...) TEST(lexer, _name) {tast_body(_token, _code, line, column __VA_OPT__(,) __VA_ARGS__);}

template <typename ... supposed>
void tast_body(candy_tokens_t token, const char code[], size_t line, size_t column, supposed ... value) {
  uint8_t buffer[1026] = {0x00, 0x04};
  struct candy_lexer *lex = candy_lexer_create(code, (struct candy_view *)buffer);
  candy_meta_t meta;
  candy_tokens_t type = candy_lexer_curr(lex, &meta);
  EXPECT_EQ(type, token);
  EXPECT_EQ(line, candy_lexer_dbginfo(lex)->line);
  EXPECT_EQ(column, candy_lexer_dbginfo(lex)->column);
  if constexpr(sizeof...(value)) {
    auto val = std::get<0>(std::make_tuple(value ...));
    if constexpr (std::is_same<decltype(val), std::string_view>::value) {
      EXPECT_EQ(candy_wrap_get_string(meta.wrap)->size, val.size());
      EXPECT_EQ(memcmp((char *)candy_wrap_get_string(meta.wrap)->data, val.data(), candy_wrap_get_string(meta.wrap)->size), 0);
      candy_wrap_delete(&meta.wrap);
    }
    else if constexpr (std::is_same<decltype(val), candy_integer_t>::value) {
      EXPECT_EQ(meta.i, val);
    }
    else if constexpr (std::is_same<decltype(val), candy_float_t>::value) {
      EXPECT_PRED_FORMAT2(::testing::internal::CmpHelperFloatingPointEQ<candy_float_t>, meta.f, val);
    }
    else {
      candy_assert(0, "unknown type");
    }
  }
  candy_lexer_delete(&lex);
}

namespace std {

TEST_LEXER(comment_0, CANDY_TK_NONE, "#", 1, 1);

TEST_LEXER(comment_1, CANDY_TK_NONE, "# hello world\r\n", 2, 0);

TEST_LEXER(comment_2, CANDY_TK_NONE,
  "# hello world\n"
  "# hi\n",
  3, 0
);

TEST_LEXER(string_0, CANDY_TK_CST_STRING,
  "\"\"",
  1, 2,
  ""sv
);

TEST_LEXER(string_1, CANDY_TK_CST_STRING,
  "\"hello world\"",
  1, 13,
  "hello world"sv
);

TEST_LEXER(string_2, CANDY_TK_CST_STRING,
  "\"\\\\\"",/* "\\" */
  1, 4,
  "\\"sv
);

TEST_LEXER(string_3, CANDY_TK_CST_STRING,
  "\"\\\"\"",/* "\"" */
  1, 4,
  "\""sv
);

TEST_LEXER(string_4, CANDY_TK_CST_STRING,
  "\"\\hello world\"",/* "\hello world" */
  1, 14,
  "\\hello world"sv
);

TEST_LEXER(string_hex, CANDY_TK_CST_STRING,
  "\"\\x68\\x65\\x6C\\x6C\\x6F\\x20\\x77\\x6F\\x72\\x6C\\x64\"",/* "\x68\x65\x6C\x6C\x6F\x20\x77\x6F\x72\x6C\x64" */
  1, 46,
  "hello world"sv
);

TEST_LEXER(string_oct, CANDY_TK_CST_STRING,
  "\"\\150\\145\\154\\154\\157\\40\\167\\157\\162\\154\\144\\0\"",/* "\150\145\154\154\157\40\167\157\162\154\144\0" */
  1, 47,
  "hello world\0"sv
);

TEST_LEXER(string_multiline_0, CANDY_TK_CST_STRING,
  "''''''",
  1, 6,
  ""sv
);

TEST_LEXER(string_multiline_1, CANDY_TK_CST_STRING,
  "'''\n"
  "hello\n"
  "world\n"
  "'''",
  4, 3,
  "\nhello\nworld\n"sv
);

TEST_LEXER(integer_0, CANDY_TK_CST_INTEGER,    "114514", 1, 6,    114514);
TEST_LEXER(integer_1, CANDY_TK_CST_INTEGER,   "+114514", 1, 7,   +114514);
TEST_LEXER(integer_2, CANDY_TK_CST_INTEGER,   "-114514", 1, 7,   -114514);
TEST_LEXER(hex_0,     CANDY_TK_CST_INTEGER,  "0x114514", 1, 8,  0x114514);
TEST_LEXER(hex_1,     CANDY_TK_CST_INTEGER, "+0x114514", 1, 9, +0x114514);
TEST_LEXER(hex_2,     CANDY_TK_CST_INTEGER, "-0x114514", 1, 9, -0x114514);

TEST_LEXER(float_0, CANDY_TK_CST_FLOAT,      "3.1415926", 1,  9,     3.1415926);
TEST_LEXER(float_1, CANDY_TK_CST_FLOAT,     "+3.1415926", 1, 10,    +3.1415926);
TEST_LEXER(float_2, CANDY_TK_CST_FLOAT,     "-3.1415926", 1, 10,    -3.1415926);
TEST_LEXER(sci_0,   CANDY_TK_CST_FLOAT,   "0.31415926e1", 1, 12,  0.31415926e1);
TEST_LEXER(sci_1,   CANDY_TK_CST_FLOAT,  "+0.31415926e1", 1, 13, +0.31415926e1);
TEST_LEXER(sci_2,   CANDY_TK_CST_FLOAT,  "-0.31415926e1", 1, 13, -0.31415926e1);
TEST_LEXER(sci_3,   CANDY_TK_CST_FLOAT,   "314.15926e-2", 1, 12,  314.15926e-2);
TEST_LEXER(sci_4,   CANDY_TK_CST_FLOAT,  "+314.15926e-2", 1, 13, +314.15926e-2);
TEST_LEXER(sci_5,   CANDY_TK_CST_FLOAT,  "-314.15926e-2", 1, 13, -314.15926e-2);

#define CANDY_KW_TEST
#include "src/core/candy_keyword.list"

TEST_LEXER(CANDY_TK_DEL_LPAREN , CANDY_TK_DEL_LPAREN ,  "(", 1, 1);
TEST_LEXER(CANDY_TK_DEL_RPAREN , CANDY_TK_DEL_RPAREN ,  ")", 1, 1);
TEST_LEXER(CANDY_TK_DEL_LBRACE , CANDY_TK_DEL_LBRACE ,  "[", 1, 1);
TEST_LEXER(CANDY_TK_DEL_RBRACE , CANDY_TK_DEL_RBRACE ,  "]", 1, 1);
TEST_LEXER(CANDY_TK_OPE_BITAND , CANDY_TK_OPE_BITAND ,  "&", 1, 1);
TEST_LEXER(CANDY_TK_OPE_BITOR  , CANDY_TK_OPE_BITOR  ,  "|", 1, 1);
TEST_LEXER(CANDY_TK_OPE_BITNOT , CANDY_TK_OPE_BITNOT ,  "~", 1, 1);
TEST_LEXER(CANDY_TK_OPE_BITXOR , CANDY_TK_OPE_BITXOR ,  "^", 1, 1);
TEST_LEXER(CANDY_TK_OPE_MOD    , CANDY_TK_OPE_MOD    ,  "%", 1, 1);
TEST_LEXER(CANDY_TK_OPE_ADD    , CANDY_TK_OPE_ADD    ,  "+", 1, 1);
TEST_LEXER(CANDY_TK_OPE_SUB    , CANDY_TK_OPE_SUB    ,  "-", 1, 1);
TEST_LEXER(CANDY_TK_OPE_MUL    , CANDY_TK_OPE_MUL    ,  "*", 1, 1);
TEST_LEXER(CANDY_TK_OPE_DIV    , CANDY_TK_OPE_DIV    ,  "/", 1, 1);
TEST_LEXER(CANDY_TK_OPE_ASSIGN , CANDY_TK_OPE_ASSIGN ,  "=", 1, 1);
TEST_LEXER(CANDY_TK_OPE_GREATER, CANDY_TK_OPE_GREATER,  ">", 1, 1);
TEST_LEXER(CANDY_TK_OPE_LESS   , CANDY_TK_OPE_LESS   ,  "<", 1, 1);
TEST_LEXER(CANDY_TK_OPE_EXP    , CANDY_TK_OPE_EXP    , "**", 1, 2);
TEST_LEXER(CANDY_TK_OPE_FLRDIV , CANDY_TK_OPE_FLRDIV , "//", 1, 2);
TEST_LEXER(CANDY_TK_OPE_MODASS , CANDY_TK_OPE_MODASS , "%=", 1, 2);
TEST_LEXER(CANDY_TK_OPE_NEQUAL , CANDY_TK_OPE_NEQUAL , "!=", 1, 2);
TEST_LEXER(CANDY_TK_OPE_ADDASS , CANDY_TK_OPE_ADDASS , "+=", 1, 2);
TEST_LEXER(CANDY_TK_OPE_SUBASS , CANDY_TK_OPE_SUBASS , "-=", 1, 2);
TEST_LEXER(CANDY_TK_OPE_MULASS , CANDY_TK_OPE_MULASS , "*=", 1, 2);
TEST_LEXER(CANDY_TK_OPE_DIVASS , CANDY_TK_OPE_DIVASS , "/=", 1, 2);
TEST_LEXER(CANDY_TK_OPE_EQUAL  , CANDY_TK_OPE_EQUAL  , "==", 1, 2);
TEST_LEXER(CANDY_TK_OPE_GEQUAL , CANDY_TK_OPE_GEQUAL , ">=", 1, 2);
TEST_LEXER(CANDY_TK_OPE_LEQUAL , CANDY_TK_OPE_LEQUAL , "<=", 1, 2);
TEST_LEXER(CANDY_TK_OPE_RSHIFT , CANDY_TK_OPE_RSHIFT , ">>", 1, 2);
TEST_LEXER(CANDY_TK_OPE_LSHIFT , CANDY_TK_OPE_LSHIFT , "<<", 1, 2);

} /* namespace std */
