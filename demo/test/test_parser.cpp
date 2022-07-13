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
#include "src/core/candy_parser.h"

// extern "C" {

// ast_node_t _ast_node_create(int8_t token, uint8_t meta[], ast_node_t l, ast_node_t r);
// int _ast_node_delete(ast_node_t *node);

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
  auto parser = candy_parser_create("1 + 2");
  candy_parser_delete(&parser);
}

TEST(parser, exp_add_1) {
  auto parser = candy_parser_create("(1 + 2)");
  candy_parser_delete(&parser);
}

TEST(parser, exp_sub_0) {
  auto parser = candy_parser_create("1 - 2");
  candy_parser_delete(&parser);
}

TEST(parser, exp_sub_1) {
  auto parser = candy_parser_create("(1 - 2)");
  candy_parser_delete(&parser);
}

TEST(parser, exp_mul_0) {
  auto parser = candy_parser_create("1 * 2");
  candy_parser_delete(&parser);
}

TEST(parser, exp_mul_1) {
  auto parser = candy_parser_create("(1 * 2)");
  candy_parser_delete(&parser);
}

TEST(parser, exp_div_0) {
  auto parser = candy_parser_create("1 / 2");
  candy_parser_delete(&parser);
}

TEST(parser, exp_div_1) {
  auto parser = candy_parser_create("(1 / 2)");
  candy_parser_delete(&parser);
}

TEST(parser, exp_4) {
  const char exp[] = "(((1 + (2 * 3)/(4.5 + 5))*(6 - 7) + (8 + 9) * 10)/11) - 12";
  candy_parser_t parser = candy_parser_create(exp);
  candy_parser_delete(&parser);
}
