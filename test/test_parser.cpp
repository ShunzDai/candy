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
#include "test_common.h"
#include "src/candy_parser.h"
#include "src/candy_io.h"

#define PARSER_TEST(_name, _exp) TEST(parser, _name) {test_body(_exp);}

static void test_body(const char exp[]) {
  info_str info = {exp, (int)strlen(exp) + 1, 0};
  candy_buffer_t buffer;
  candy_buffer_init(&buffer, CANDY_ATOMIC_BUFFER_SIZE, sizeof(char));
  candy_io_t io;
  candy_io_set_input(&io, &buffer, _string_reader, &info);
  candy_parse(&buffer, _string_reader, &info);
  candy_buffer_deinit(&buffer);
}

PARSER_TEST(exp_add_0, "1 + 2")
PARSER_TEST(exp_add_1, "(1 + 2)")
PARSER_TEST(exp_sub_0, "1 - 2")
PARSER_TEST(exp_sub_1, "(1 - 2)")
PARSER_TEST(exp_mul_0, "1 * 2")
PARSER_TEST(exp_mul_1, "(1 * 2)")
PARSER_TEST(exp_div_0, "1 / 2")
PARSER_TEST(exp_div_1, "(1 / 2)")
PARSER_TEST(exp, "(((-0xa + (-2e+3 *+2e-2)/(-4.5e+5 +-1.5e-2))*(6.4 --7.6) + (+8.4 + 9) * 10)/11) - 12")
