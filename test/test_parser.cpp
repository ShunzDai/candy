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
#include "src/candy_parser.h"
#include "src/candy_reader.h"

#define PARSER_TEST(_name, _exp) TEST(parser, _name) {test_body(_exp);}

static void test_body(const char exp[]) {
  str_info info = {exp, strlen(exp), 0};
  candy_buffer_t *io = candy_buffer_create(CANDY_ATOMIC_IO_SIZE, sizeof(char), true);
  EXPECT_EQ(candy_parse(io, string_reader, &info) == nullptr, false);
  candy_buffer_delete(&io);
}

PARSER_TEST(exp_add_0, "a = 1 + 2")
PARSER_TEST(exp_add_1, "a = (1 + 2)")
PARSER_TEST(exp_sub_0, "a = 1 - 2")
PARSER_TEST(exp_sub_1, "a = (1 - 2)")
PARSER_TEST(exp_mul_0, "a = 1 * 2")
PARSER_TEST(exp_mul_1, "a = (1 * 2)")
PARSER_TEST(exp_div_0, "a = 1 / 2")
PARSER_TEST(exp_div_1, "a = (1 / 2)")
PARSER_TEST(exp, "a = (((-0xa + (-2e+3 *+2e-2)/(-4.5e+5 +-1.5e-2))*(6.4 --7.6) + (+8.4 + 9) * 10)/11) - 12")
