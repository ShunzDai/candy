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
#include "src/struct/candy_object.h"
#include "src/method/candy_standard.h"
#include "src/core/candy_parser.h"

#define TEST_LEXER(name, type, in, out)\
TEST(lexer, name){\
  candy_lexer_t lex = candy_lexer_create((char *)in);\
  candy_pack_t pack = NULL;\
  EXPECT_EQ(candy_lexer_get_token(lex, &pack), type);\
  if (pack != NULL){\
    EXPECT_EQ(strncmp((char *)candy_pack_get_string(pack).data, out, candy_pack_get_string(pack).size), 0);\
    pack = candy_pack_delete(pack);\
  }\
  lex = candy_lexer_delete(lex);\
}

TEST_LEXER(singleline_comment_0, CANDY_TOKEN_NULL,
  "#\n",
  ""
);

TEST_LEXER(singleline_comment_1, CANDY_TOKEN_NULL,
  "# hello world\r\n",
  ""
);

TEST_LEXER(singleline_comment_2, CANDY_TOKEN_NULL,
  "# hello world\n"
  "# hi\n",
  ""
);

TEST_LEXER(multiline_comment_0, CANDY_TOKEN_NULL,
  "''''''\n",
  ""
);

TEST_LEXER(multiline_comment_1, CANDY_TOKEN_NULL,
  "'''\n"
  "hello\n"
  "world\n"
  "'''\n",
  "\nhello\nworld\n"
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
