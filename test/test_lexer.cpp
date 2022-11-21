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
#include "src/candy_wrap.h"
#include "src/candy_lexer.h"
#include <string>

#define TEST_LEXER(_name, _token, _code, ...) TEST(lexer, _name) {tast_body(_token, _code __VA_OPT__(,) __VA_ARGS__);}

using namespace std;

struct reader_ud {
  int offset;
  const char *code;
};

static int reader(char *buff, int size, void *ud) {
  reader_ud *info = (reader_ud *)ud;
  int len = strlen(info->code) + 1;
  len = (size > len - info->offset) ? (len - info->offset) : size;
  memcpy(buff, info->code + info->offset, len);
  info->offset += len;
  return len;
}

template <typename ... supposed>
void tast_body(candy_tokens_t token, const char code[], supposed ... value) {
  reader_ud info = {0, code};
  candy_lexer_t *lex = candy_lexer_create(reader, &info);
  candy_wrap_t wrap;
  candy_tokens_t type = candy_lexer_next(lex, &wrap);
  EXPECT_EQ(type, token);
  if constexpr(sizeof...(value)) {
    auto val = std::get<0>(std::make_tuple(value ...));
    if constexpr (std::is_same<decltype(val), std::string_view>::value) {
      int size;
      char *str = candy_wrap_get_string(&wrap, &size);
      EXPECT_EQ(memcmp(str, val.data(), size), 0);
      EXPECT_EQ(size, val.size());
      candy_wrap_deinit(&wrap);
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
  candy_lexer_delete(&lex);
}

TEST_LEXER(comment_0, CANDY_TK_NONE, "#");

TEST_LEXER(comment_1, CANDY_TK_NONE, "# hello world\r\n");

TEST_LEXER(comment_2, CANDY_TK_NONE,
  "# hello world\n"
  "# hi\n"
);

TEST_LEXER(string_0, CANDY_TK_CST_STRING,
  "\"\"",
  ""sv
);

TEST_LEXER(string_1, CANDY_TK_CST_STRING,
  "\"hello world\"",
  "hello world"sv
);

TEST_LEXER(string_2, CANDY_TK_CST_STRING,
  "\"\\\\\"",/* "\\" */
  "\\"sv
);

TEST_LEXER(string_3, CANDY_TK_CST_STRING,
  "\"\\\"\"",/* "\"" */
  "\""sv
);

TEST_LEXER(string_4, CANDY_TK_CST_STRING,
  "\"\\hello world\"",/* "\hello world" */
  "\\hello world"sv
);

TEST_LEXER(string_hex, CANDY_TK_CST_STRING,
  "\"\\x68\\x65\\x6C\\x6C\\x6F\\x20\\x77\\x6F\\x72\\x6C\\x64\"",/* "\x68\x65\x6C\x6C\x6F\x20\x77\x6F\x72\x6C\x64" */
  "hello world"sv
);

TEST_LEXER(string_oct, CANDY_TK_CST_STRING,
  "\"\\150\\145\\154\\154\\157\\40\\167\\157\\162\\154\\144\\0\"",/* "\150\145\154\154\157\40\167\157\162\154\144\0" */
  "hello world\0"sv
);

TEST_LEXER(string_multiline_0, CANDY_TK_CST_STRING,
  "''''''",
  ""sv
);

TEST_LEXER(string_multiline_1, CANDY_TK_CST_STRING,
  "'''\n"
  "hello\n"
  "world\n"
  "'''",
  "\nhello\nworld\n"sv
);

TEST_LEXER(integer, CANDY_TK_CST_INTEGER,       "114514",       114514);
TEST_LEXER(hex,     CANDY_TK_CST_INTEGER,   "0xAbCd1234",   0xAbCd1234);
TEST_LEXER(float,   CANDY_TK_CST_FLOAT  ,    "3.1415926",    3.1415926);
TEST_LEXER(sci_0,   CANDY_TK_CST_FLOAT  , "0.31415926e1", 0.31415926e1);
TEST_LEXER(sci_1,   CANDY_TK_CST_FLOAT  , "314.15926e-2", 314.15926e-2);

#define CANDY_KW_TEST
#include "src/candy_keyword.list"

TEST_LEXER(CANDY_TK_DEL_LPAREN , CANDY_TK_DEL_LPAREN ,  "(");
TEST_LEXER(CANDY_TK_DEL_RPAREN , CANDY_TK_DEL_RPAREN ,  ")");
TEST_LEXER(CANDY_TK_DEL_LBRACE , CANDY_TK_DEL_LBRACE ,  "[");
TEST_LEXER(CANDY_TK_DEL_RBRACE , CANDY_TK_DEL_RBRACE ,  "]");
TEST_LEXER(CANDY_TK_OPE_BITAND , CANDY_TK_OPE_BITAND ,  "&");
TEST_LEXER(CANDY_TK_OPE_BITOR  , CANDY_TK_OPE_BITOR  ,  "|");
TEST_LEXER(CANDY_TK_OPE_BITNOT , CANDY_TK_OPE_BITNOT ,  "~");
TEST_LEXER(CANDY_TK_OPE_BITXOR , CANDY_TK_OPE_BITXOR ,  "^");
TEST_LEXER(CANDY_TK_OPE_MOD    , CANDY_TK_OPE_MOD    ,  "%");
TEST_LEXER(CANDY_TK_OPE_ADD    , CANDY_TK_OPE_ADD    ,  "+");
TEST_LEXER(CANDY_TK_OPE_SUB    , CANDY_TK_OPE_SUB    ,  "-");
TEST_LEXER(CANDY_TK_OPE_MUL    , CANDY_TK_OPE_MUL    ,  "*");
TEST_LEXER(CANDY_TK_OPE_DIV    , CANDY_TK_OPE_DIV    ,  "/");
TEST_LEXER(CANDY_TK_OPE_ASSIGN , CANDY_TK_OPE_ASSIGN ,  "=");
TEST_LEXER(CANDY_TK_OPE_GREATER, CANDY_TK_OPE_GREATER,  ">");
TEST_LEXER(CANDY_TK_OPE_LESS   , CANDY_TK_OPE_LESS   ,  "<");
TEST_LEXER(CANDY_TK_OPE_EXP    , CANDY_TK_OPE_EXP    , "**");
TEST_LEXER(CANDY_TK_OPE_FLRDIV , CANDY_TK_OPE_FLRDIV , "//");
TEST_LEXER(CANDY_TK_OPE_MODASS , CANDY_TK_OPE_MODASS , "%=");
TEST_LEXER(CANDY_TK_OPE_NEQUAL , CANDY_TK_OPE_NEQUAL , "!=");
TEST_LEXER(CANDY_TK_OPE_ADDASS , CANDY_TK_OPE_ADDASS , "+=");
TEST_LEXER(CANDY_TK_OPE_SUBASS , CANDY_TK_OPE_SUBASS , "-=");
TEST_LEXER(CANDY_TK_OPE_MULASS , CANDY_TK_OPE_MULASS , "*=");
TEST_LEXER(CANDY_TK_OPE_DIVASS , CANDY_TK_OPE_DIVASS , "/=");
TEST_LEXER(CANDY_TK_OPE_EQUAL  , CANDY_TK_OPE_EQUAL  , "==");
TEST_LEXER(CANDY_TK_OPE_GEQUAL , CANDY_TK_OPE_GEQUAL , ">=");
TEST_LEXER(CANDY_TK_OPE_LEQUAL , CANDY_TK_OPE_LEQUAL , "<=");
TEST_LEXER(CANDY_TK_OPE_RSHIFT , CANDY_TK_OPE_RSHIFT , ">>");
TEST_LEXER(CANDY_TK_OPE_LSHIFT , CANDY_TK_OPE_LSHIFT , "<<");

TEST(lexer, complex_expression) {
  reader_ud info = {0,
    "while True:\n"
    "  print(\"hello world\\n\")\n"
  };
  candy_lexer_t *lex = candy_lexer_create(reader, &info);
  candy_wrap_t wrap;
  EXPECT_EQ(candy_lexer_next(lex, &wrap), CANDY_TK_KW_while);
  EXPECT_EQ(candy_lexer_next(lex, &wrap), CANDY_TK_KW_True);
  EXPECT_EQ(candy_lexer_next(lex, &wrap), CANDY_TK_DEL_COLON);
  EXPECT_EQ(candy_lexer_next(lex, &wrap), CANDY_TK_IDENT);
  EXPECT_EQ(candy_lexer_next(lex, &wrap), CANDY_TK_DEL_LPAREN);
  EXPECT_EQ(candy_lexer_next(lex, &wrap), CANDY_TK_CST_STRING);
  EXPECT_EQ(candy_lexer_next(lex, &wrap), CANDY_TK_DEL_RPAREN);
  EXPECT_EQ(candy_lexer_next(lex, &wrap), CANDY_TK_NONE);
  candy_wrap_deinit(&wrap);
  candy_lexer_delete(&lex);
}
