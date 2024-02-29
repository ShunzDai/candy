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
#include "test.h"
#include "src/candy_io.h"
#include "src/candy_lexer.h"
#include "src/candy_reader.h"
#include <string>

#define TEST_NORMAL(_name, _token, _exp, ...) TEST(lexer, _name) { tast_body<_token,  0>(_exp __VA_OPT__(,) __VA_ARGS__); }
#define TEST_ASSERT(_name, _exp, ...) TEST(lexer, _name) { tast_body<TK_EOS, -1>(_exp __VA_OPT__(,) __VA_ARGS__); }

using namespace std;

template <typename ... supposed>
static void test_normal(const candy_wrap_t& wrap, const supposed & ... value) {
  auto val = std::get<0>(std::make_tuple(value ...));
  if constexpr (std::is_same<decltype(val), std::string_view>::value) {
    auto *str = candy_wrap_get_string(&wrap);
    EXPECT_EQ(candy_wrap_size(&wrap), val.size());
    EXPECT_MEMEQ(str, val.data(), val.size());
  }
  else if constexpr (std::is_integral<decltype(val)>::value) {
    EXPECT_EQ(*candy_wrap_get_integer(&wrap), val);
  }
  else if constexpr (std::is_floating_point<decltype(val)>::value) {
    EXPECT_PRED_FORMAT2(::testing::internal::CmpHelperFloatingPointEQ<candy_float_t>, *candy_wrap_get_float(&wrap), val);
  }
  else {
    static_assert(!std::is_same<decltype(val), decltype(val)>::value);
  }
}

template <typename ... supposed>
static void test_assert(const candy_io_t& io, const supposed & ... value) {
  auto val = std::get<0>(std::make_tuple(value ...));
  static_assert(std::is_same<decltype(val), std::string_view>::value);
  EXPECT_EQ(strlen(candy_wrap_get_string(&io.buff)), val.size());
  EXPECT_MEMEQ(candy_wrap_get_string(&io.buff), val.data(), val.size() + 1);
}

template <candy_tokens_t token, int assert, typename ... supposed>
static void tast_body(const char exp[], const supposed & ... value) {
  candy_io_t io{};
  candy_lexer_t lex{};
  str_info info{exp, strlen(exp), 0};
  candy_io_init(&io);
  candy_lexer_init(&lex, &io, string_reader, &info);
  candy_wrap_t wrap{};

  ASSERT_EQ(candy_io_try_catch(&io, (candy_try_catch_cb_t)+[](candy_lexer_t *self, candy_wrap_t *wrap) {
    EXPECT_EQ(candy_lexer_lookahead(self), token);
    *wrap = *candy_lexer_next(self);
    EXPECT_EQ(candy_lexer_lookahead(self), TK_EOS);
  }, &lex, &wrap), assert);

  if constexpr(assert == 0 && sizeof...(value))
    test_normal(wrap, value ...);
  else if constexpr (assert == -1 && sizeof...(value))
    test_assert(io, value ...);

  candy_wrap_deinit(&wrap);
  candy_lexer_deinit(&lex);
  candy_io_deinit(&io);
}

TEST_ASSERT(invalid_0, "0x", "lexical error: invalid hexadecimal number"sv)
TEST_ASSERT(invalid_1, "0b", "lexical error: invalid binary number"sv)
TEST_ASSERT(invalid_2, "0b1e", "lexical error: invalid float number"sv)

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
  "\"\\hello world\"",/* "\hello world" */
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

TEST_NORMAL(vararg, TK_VARARG, "...")

#define CANDY_KW_TEST
#include "src/candy_keyword.list"

TEST_NORMAL(ident_0, TK_IDENT, "i")
TEST_NORMAL(ident_1, TK_IDENT, "ifif")

TEST_NORMAL(TK_LPAREN , TK_LPAREN ,  "(")
TEST_NORMAL(TK_RPAREN , TK_RPAREN ,  ")")
TEST_NORMAL(TK_COMMA  , TK_COMMA  ,  ",")
TEST_NORMAL(TK_DOT    , TK_DOT    ,  ".")
TEST_NORMAL(TK_COLON  , TK_COLON  ,  ":")
TEST_NORMAL(TK_LBRACE , TK_LBRACE ,  "{")
TEST_NORMAL(TK_RBRACE , TK_RBRACE ,  "}")
TEST_NORMAL(TK_LSQUARE, TK_LSQUARE,  "[")
TEST_NORMAL(TK_RSQUARE, TK_RSQUARE,  "]")
TEST_NORMAL(TK_AMPER  , TK_AMPER  ,  "&")
TEST_NORMAL(TK_VERT   , TK_VERT   ,  "|")
TEST_NORMAL(TK_TILDE  , TK_TILDE  ,  "~")
TEST_NORMAL(TK_CARET  , TK_CARET  ,  "^")
TEST_NORMAL(TK_PERCENT, TK_PERCENT,  "%")
TEST_NORMAL(TK_PLUS   , TK_PLUS   ,  "+")
TEST_NORMAL(TK_MINUS  , TK_MINUS  ,  "-")
TEST_NORMAL(TK_ASTE   , TK_ASTE   ,  "*")
TEST_NORMAL(TK_SLASH  , TK_SLASH  ,  "/")
TEST_NORMAL(TK_ASSIGN , TK_ASSIGN ,  "=")
TEST_NORMAL(TK_GREATER, TK_GREATER,  ">")
TEST_NORMAL(TK_LESS   , TK_LESS   ,  "<")
TEST_NORMAL(TK_EXP    , TK_EXP    , "**")
TEST_NORMAL(TK_FLRDIV , TK_FLRDIV , "//")
TEST_NORMAL(TK_MODASS , TK_MODASS , "%=")
TEST_NORMAL(TK_NEQUAL , TK_NEQUAL , "!=")
TEST_NORMAL(TK_ADDASS , TK_ADDASS , "+=")
TEST_NORMAL(TK_SUBASS , TK_SUBASS , "-=")
TEST_NORMAL(TK_MULASS , TK_MULASS , "*=")
TEST_NORMAL(TK_DIVASS , TK_DIVASS , "/=")
TEST_NORMAL(TK_EQUAL  , TK_EQUAL  , "==")
TEST_NORMAL(TK_GEQUAL , TK_GEQUAL , ">=")
TEST_NORMAL(TK_LEQUAL , TK_LEQUAL , "<=")
TEST_NORMAL(TK_RSHIFT , TK_RSHIFT , ">>")
TEST_NORMAL(TK_LSHIFT , TK_LSHIFT , "<<")

TEST(lexer, file_system) {
  FILE *f = fopen("../test/test_lexer.cdy", "r");
  candy_io_t io{};
  candy_lexer_t lex{};
  file_info info{f};
  candy_io_init(&io);
  candy_lexer_init(&lex, &io, file_reader, &info);
  ASSERT_EQ(candy_io_try_catch(&io, (candy_try_catch_cb_t)+[](candy_lexer_t *self, void *ud) {
    EXPECT_EQ(candy_lexer_lookahead(self), TK_def);
    candy_wrap_deinit((candy_wrap_t *)candy_lexer_next(self));
    EXPECT_EQ(candy_lexer_lookahead(self), TK_IDENT);
    candy_wrap_deinit((candy_wrap_t *)candy_lexer_next(self));
    EXPECT_EQ(candy_lexer_lookahead(self), '(');
    candy_wrap_deinit((candy_wrap_t *)candy_lexer_next(self));
    EXPECT_EQ(candy_lexer_lookahead(self), TK_IDENT);
    candy_wrap_deinit((candy_wrap_t *)candy_lexer_next(self));
    EXPECT_EQ(candy_lexer_lookahead(self), ')');
    candy_wrap_deinit((candy_wrap_t *)candy_lexer_next(self));
    EXPECT_EQ(candy_lexer_lookahead(self), ':');
    candy_wrap_deinit((candy_wrap_t *)candy_lexer_next(self));
    EXPECT_EQ(candy_lexer_lookahead(self), TK_IDENT);
    candy_wrap_deinit((candy_wrap_t *)candy_lexer_next(self));
    EXPECT_EQ(candy_lexer_lookahead(self), '(');
    candy_wrap_deinit((candy_wrap_t *)candy_lexer_next(self));
    EXPECT_EQ(candy_lexer_lookahead(self), TK_IDENT);
    candy_wrap_deinit((candy_wrap_t *)candy_lexer_next(self));
    EXPECT_EQ(candy_lexer_lookahead(self), '+');
    candy_wrap_deinit((candy_wrap_t *)candy_lexer_next(self));
    EXPECT_EQ(candy_lexer_lookahead(self), TK_STRING);
    candy_wrap_deinit((candy_wrap_t *)candy_lexer_next(self));
    EXPECT_EQ(candy_lexer_lookahead(self), ')');
    candy_wrap_deinit((candy_wrap_t *)candy_lexer_next(self));
    EXPECT_EQ(candy_lexer_lookahead(self), TK_return);
    candy_wrap_deinit((candy_wrap_t *)candy_lexer_next(self));
    EXPECT_EQ(candy_lexer_lookahead(self), TK_INTEGER);
    candy_wrap_deinit((candy_wrap_t *)candy_lexer_next(self));
    EXPECT_EQ(candy_lexer_lookahead(self), TK_if);
    candy_wrap_deinit((candy_wrap_t *)candy_lexer_next(self));
    EXPECT_EQ(candy_lexer_lookahead(self), TK_IDENT);
    candy_wrap_deinit((candy_wrap_t *)candy_lexer_next(self));
    EXPECT_EQ(candy_lexer_lookahead(self), binary_ope('=', '='));
    candy_wrap_deinit((candy_wrap_t *)candy_lexer_next(self));
    EXPECT_EQ(candy_lexer_lookahead(self), TK_STRING);
    candy_wrap_deinit((candy_wrap_t *)candy_lexer_next(self));
    EXPECT_EQ(candy_lexer_lookahead(self), ':');
    candy_wrap_deinit((candy_wrap_t *)candy_lexer_next(self));
    EXPECT_EQ(candy_lexer_lookahead(self), TK_IDENT);
    candy_wrap_deinit((candy_wrap_t *)candy_lexer_next(self));
    EXPECT_EQ(candy_lexer_lookahead(self), '(');
    candy_wrap_deinit((candy_wrap_t *)candy_lexer_next(self));
    EXPECT_EQ(candy_lexer_lookahead(self), TK_STRING);
    candy_wrap_deinit((candy_wrap_t *)candy_lexer_next(self));
    EXPECT_EQ(candy_lexer_lookahead(self), ')');
    candy_wrap_deinit((candy_wrap_t *)candy_lexer_next(self));
    EXPECT_EQ(candy_lexer_lookahead(self), TK_EOS);
  }, &lex, nullptr), 0);
  candy_lexer_deinit(&lex);
  candy_io_deinit(&io);
  fclose(info.f);
}


