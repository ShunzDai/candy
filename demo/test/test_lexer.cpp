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

#define TEST_LEXER(_name, _type, _in, _out)\
TEST(lexer, _name){\
  candy_lexer_t lex = candy_lexer_create((char *)_in);\
  candy_meta_t meta;\
  int t = candy_lexer_curr(lex, &meta);\
  EXPECT_EQ(t, _type);\
  if (t == CANDY_TK_CST_STRING){\
    EXPECT_EQ(candy_wrap_get_string(meta.wrap)->size, sizeof(_out) - 1);\
    EXPECT_EQ(memcmp((char *)candy_wrap_get_string(meta.wrap)->data, _out, candy_wrap_get_string(meta.wrap)->size), 0);\
    candy_wrap_delete(&meta.wrap, NULL);\
  }\
  candy_lexer_delete(&lex);\
}

TEST_LEXER(comment_0, CANDY_TK_NONE,
  "#\n",
  ""
);

TEST_LEXER(comment_1, CANDY_TK_NONE,
  "# hello world\r\n",
  ""
);

TEST_LEXER(comment_2, CANDY_TK_NONE,
  "# hello world\n"
  "# hi\n",
  ""
);

TEST_LEXER(string_0, CANDY_TK_CST_STRING,
  "\"\"",
  ""
);

TEST_LEXER(string_1, CANDY_TK_CST_STRING,
  "\"hello world\"",
  "hello world"
);

TEST_LEXER(string_2, CANDY_TK_CST_STRING,
  "\"\\\\\"",/* "\\" */
  "\\"
);

TEST_LEXER(string_3, CANDY_TK_CST_STRING,
  "\"\\\"\"",/* "\"" */
  "\""
);

TEST_LEXER(string_4, CANDY_TK_CST_STRING,
  "\"\\hello world\"",/* "\hello world" */
  "\\hello world"
);

TEST_LEXER(string_hex, CANDY_TK_CST_STRING,
  "\"\\x68\\x65\\x6C\\x6C\\x6F\\x20\\x77\\x6F\\x72\\x6C\\x64\"",/* "\x68\x65\x6C\x6C\x6F\x20\x77\x6F\x72\x6C\x64" */
  "hello world"
);

TEST_LEXER(string_oct, CANDY_TK_CST_STRING,
  "\"\\150\\145\\154\\154\\157\\40\\167\\157\\162\\154\\144\"",/* "\x68\x65\x6C\x6C\x6F\x20\x77\x6F\x72\x6C\x64" */
  "hello world"
);

TEST_LEXER(string_multiline_0, CANDY_TK_CST_STRING,
  "''''''\n",
  ""
);

TEST_LEXER(string_multiline_1, CANDY_TK_CST_STRING,
  "'''\n"
  "hello\n"
  "world\n"
  "'''\n",
  "\nhello\nworld\n"
);
