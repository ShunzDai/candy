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
#include "src/core/candy_parser.h"

#define TEST_LEXER(name, type, in, out)\
TEST(lexer, name){\
  candy_lexer_t lex = candy_lexer_create((char *)in);\
  candy_wrap_t pack = NULL;\
  EXPECT_EQ(candy_lexer_get_token(lex, &pack), type);\
  if (pack != NULL){\
    EXPECT_EQ(candy_wrap_get_string(pack)->size, sizeof(out) - 1);\
    EXPECT_EQ(memcmp((char *)candy_wrap_get_string(pack)->data, out, candy_wrap_get_string(pack)->size), 0);\
    candy_wrap_delete(&pack);\
  }\
  lex = candy_lexer_delete(lex);\
}

TEST_LEXER(comment_0, CANDY_TOKEN_NULL,
  "#\n",
  ""
);

TEST_LEXER(comment_1, CANDY_TOKEN_NULL,
  "# hello world\r\n",
  ""
);

TEST_LEXER(comment_2, CANDY_TOKEN_NULL,
  "# hello world\n"
  "# hi\n",
  ""
);

TEST_LEXER(string_0, CANDY_TOKEN_CONST_STRING,
  "\"\"",
  ""
);

TEST_LEXER(string_1, CANDY_TOKEN_CONST_STRING,
  "\"hello world\"",
  "hello world"
);

TEST_LEXER(string_2, CANDY_TOKEN_CONST_STRING,
  "\"\\\\\"",/* "\\" */
  "\\"
);

TEST_LEXER(string_3, CANDY_TOKEN_CONST_STRING,
  "\"\\\"\"",/* "\"" */
  "\""
);

TEST_LEXER(string_4, CANDY_TOKEN_CONST_STRING,
  "\"\\hello world\"",/* "\hello world" */
  "\\hello world"
);

TEST_LEXER(string_hex, CANDY_TOKEN_CONST_STRING,
  "\"\\x68\\x65\\x6C\\x6C\\x6F\\x20\\x77\\x6F\\x72\\x6C\\x64\"",/* "\x68\x65\x6C\x6C\x6F\x20\x77\x6F\x72\x6C\x64" */
  "hello world"
);

TEST_LEXER(string_oct, CANDY_TOKEN_CONST_STRING,
  "\"\\150\\145\\154\\154\\157\\40\\167\\157\\162\\154\\144\"",/* "\x68\x65\x6C\x6C\x6F\x20\x77\x6F\x72\x6C\x64" */
  "hello world"
);

TEST_LEXER(string_multiline_0, CANDY_TOKEN_CONST_STRING,
  "''''''\n",
  ""
);

TEST_LEXER(string_multiline_1, CANDY_TOKEN_CONST_STRING,
  "'''\n"
  "hello\n"
  "world\n"
  "'''\n",
  "\nhello\nworld\n"
);

TEST(lexer, name){
  candy_lexer_t lex = candy_lexer_create((char *)
    "str = \"hello world\""
  );
  candy_wrap_t pack = NULL;
  EXPECT_EQ(candy_lexer_get_token(lex, &pack), CANDY_TOKEN_IDENT);
  EXPECT_EQ(candy_lexer_get_token(lex, &pack), CANDY_TOKEN_OPERATOR_ASSIGN);
  EXPECT_EQ(candy_lexer_get_token(lex, &pack), CANDY_TOKEN_CONST_STRING);
  if (pack != NULL){
    EXPECT_EQ(strncmp((char *)candy_wrap_get_string(pack)->data, "hello world", candy_wrap_get_string(pack)->size), 0);
    candy_wrap_delete(&pack);
  }
  lex = candy_lexer_delete(lex);
}
