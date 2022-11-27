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

// extern "C" {

// struct ast_node *_ast_node_create(int8_t token, uint8_t meta[], struct ast_node *l, struct ast_node *r);
// int _ast_node_delete(struct ast_node **node);

// }

// TEST(parser, tree_1) {
//   auto root = _ast_node_create(0, NULL, NULL, NULL);
//   _ast_node_delete(&root);
// }

// TEST(parser, tree_2) {
//   auto l = _ast_node_create(0, NULL, NULL, NULL);
//   auto r = _ast_node_create(0, NULL, NULL, NULL);
//   auto root = _ast_node_create(0, NULL, l, r);
//   _ast_node_delete(&root);
// }

// TEST(parser, tree_3) {
//   auto l = _ast_node_create(0, NULL, NULL, NULL);
//   auto root = _ast_node_create(0, NULL, l, NULL);
//   _ast_node_delete(&root);
// }

// TEST(parser, tree_4) {
//   auto ll = _ast_node_create(0, NULL, NULL, NULL);
//   auto lr = _ast_node_create(0, NULL, NULL, NULL);
//   auto rl = _ast_node_create(0, NULL, NULL, NULL);
//   auto rr = _ast_node_create(0, NULL, NULL, NULL);
//   auto l = _ast_node_create(0, NULL, ll, lr);
//   auto r = _ast_node_create(0, NULL, rl, rr);
//   auto root = _ast_node_create(0, NULL, l, r);
//   _ast_node_delete(&root);
// }

TEST(parser, exp_add_0) {
  const char exp[] = "1 + 2";
  info_str info = {exp, (int)strlen(exp) + 1, 0};
  candy_io_t io;
  candy_io_init(&io);
  candy_io_set_input(&io, _string_reader, &info);
  candy_parse(&io);
  candy_io_deinit(&io);
}

TEST(parser, exp_add_1) {
  const char exp[] = "(1 + 2)";
  info_str info = {exp, (int)strlen(exp) + 1, 0};
  candy_io_t io;
  candy_io_init(&io);
  candy_io_set_input(&io, _string_reader, &info);
  candy_parse(&io);
  candy_io_deinit(&io);
}

TEST(parser, exp_sub_0) {
  const char exp[] = "1 - 2";
  info_str info = {exp, (int)strlen(exp) + 1, 0};
  candy_io_t io;
  candy_io_init(&io);
  candy_io_set_input(&io, _string_reader, &info);
  candy_parse(&io);
  candy_io_deinit(&io);
}

TEST(parser, exp_sub_1) {
  const char exp[] = "(1 - 2)";
  info_str info = {exp, (int)strlen(exp) + 1, 0};
  candy_io_t io;
  candy_io_init(&io);
  candy_io_set_input(&io, _string_reader, &info);
  candy_parse(&io);
  candy_io_deinit(&io);
}

TEST(parser, exp_mul_0) {
  const char exp[] = "1 * 2";
  info_str info = {exp, (int)strlen(exp) + 1, 0};
  candy_io_t io;
  candy_io_init(&io);
  candy_io_set_input(&io, _string_reader, &info);
  candy_parse(&io);
  candy_io_deinit(&io);
}

TEST(parser, exp_mul_1) {
  const char exp[] = "(1 * 2)";
  info_str info = {exp, (int)strlen(exp) + 1, 0};
  candy_io_t io;
  candy_io_init(&io);
  candy_io_set_input(&io, _string_reader, &info);
  candy_parse(&io);
  candy_io_deinit(&io);
}

TEST(parser, exp_div_0) {
  const char exp[] = "1 / 2";
  info_str info = {exp, (int)strlen(exp) + 1, 0};
  candy_io_t io;
  candy_io_init(&io);
  candy_io_set_input(&io, _string_reader, &info);
  candy_parse(&io);
  candy_io_deinit(&io);
}

TEST(parser, exp_div_1) {
  const char exp[] = "(1 / 2)";
  info_str info = {exp, (int)strlen(exp) + 1, 0};
  candy_io_t io;
  candy_io_init(&io);
  candy_io_set_input(&io, _string_reader, &info);
  candy_parse(&io);
  candy_io_deinit(&io);
}

TEST(parser, exp) {
  const char exp[] = "(((-0xa + (-2e+3 *+2e-2)/(-4.5e+5 +-1.5e-2))*(6.4 --7.6) + (+8.4 + 9) * 10)/11) - 12";
  info_str info = {exp, (int)strlen(exp) + 1, 0};
  candy_io_t io;
  candy_io_init(&io);
  candy_io_set_input(&io, _string_reader, &info);
  candy_parse(&io);
  candy_io_deinit(&io);
}
