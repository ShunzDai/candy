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
#include "src/struct/candy_node.h"
#include "src/method/candy_standard.h"

TEST(node, string){
  candy_node_t node = candy_create_none((char *)"string");
  ASSERT_EQ(candy_node_checkout(node, (char *)"string"), !0);
  node = candy_node_set_string(node, (char *)"hello world");
  EXPECT_STREQ(candy_node_get_string(node), (char *)"hello world");
  node = candy_node_set_string(node, (char *)"bye world");
  EXPECT_STREQ(candy_node_get_string(node), (char *)"bye world");
  node = candy_node_delete(node);
  EXPECT_EQ((uint64_t)node, (uint64_t)NULL);
}

TEST(node, integer){
  candy_node_t node = candy_create_integer((char *)"integer", 114514);
  EXPECT_EQ(candy_node_get_integer(node), 114514);
  node = candy_node_set_integer(node, 1919810);
  EXPECT_EQ(candy_node_get_integer(node), 1919810);
  node = candy_node_delete(node);
  EXPECT_EQ((uint64_t)node, (uint64_t)NULL);
}

TEST(node, float){
  candy_node_t node = candy_create_float((char *)"float", 6.626f);
  EXPECT_FLOAT_EQ(candy_node_get_float(node), 6.626f);
  node = candy_node_set_float(node, 3.1415926f);
  EXPECT_FLOAT_EQ(candy_node_get_float(node), 3.1415926f);
  node = candy_node_delete(node);
  EXPECT_EQ((uint64_t)node, (uint64_t)NULL);
}

TEST(node, method){
  candy_node_t node = candy_create_method((char *)"method", candy_method_print);
  EXPECT_EQ((uint64_t)candy_node_get_method(node), (uint64_t)candy_method_print);
  candy_node_get_method(node)(NULL);
  node = candy_node_delete(node);
  EXPECT_EQ((uint64_t)node, (uint64_t)NULL);
}

TEST(node, cast){
  candy_node_t node = candy_create_none((char *)"cast");
  node = candy_node_set_string(node, (char *)"hello world");
  EXPECT_STREQ(candy_node_get_string(node), (char *)"hello world");
  node = candy_node_set_integer(node, 114514);
  EXPECT_EQ(candy_node_get_integer(node), 114514);
  node = candy_node_set_float(node, 3.1415926f);
  EXPECT_FLOAT_EQ(candy_node_get_float(node), 3.1415926f);
  node = candy_node_delete(node);
  EXPECT_EQ((uint64_t)node, (uint64_t)NULL);
}
