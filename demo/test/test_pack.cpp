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
#include "src/struct/candy_pack.h"
#include "src/struct/candy_object.h"
#include "src/method/candy_standard.h"

TEST(pack, none){
  candy_pack_t pack = candy_pack_none(0);
  EXPECT_EQ((uint64_t)*candy_pack_container(pack), 0);
  pack = (candy_pack_t)candy_pack_delete(pack);
  EXPECT_EQ((uint64_t)pack, (uint64_t)NULL);
}

TEST(pack, string){
  uint16_t size = 0;
  candy_pack_t pack = candy_pack_string(0, (char *)"hello world", sizeof("hello world"));
  EXPECT_STREQ(candy_pack_get_string(pack, &size), (char *)"hello world");
  EXPECT_EQ(candy_pack_size(pack), sizeof("hello world"));
  pack = candy_pack_set_string(pack, (char *)"bye world", sizeof("bye world"));
  EXPECT_STREQ(candy_pack_get_string(pack, &size), (char *)"bye world");
  EXPECT_EQ(candy_pack_size(pack), sizeof("bye world"));
  pack = (candy_pack_t)candy_pack_delete(pack);
  EXPECT_EQ((uint64_t)pack, (uint64_t)NULL);
}

TEST(pack, unsigned){
  uint16_t size = 0;
  candy_pack_t pack = candy_pack_string(0, (char *)"\x01\x02\x00\x03\x04", 5);
  EXPECT_STREQ(candy_pack_get_string(pack, &size), (char *)"\x01\x02\x00\x03\x04");
  EXPECT_EQ(candy_pack_size(pack), 5);
  pack = candy_pack_set_string(pack, (char *)"\x021\x00\x22\x23", 4);
  EXPECT_STREQ(candy_pack_get_string(pack, &size), (char *)"\x021\x00\x22\x23");
  EXPECT_EQ(candy_pack_size(pack), 4);
  pack = (candy_pack_t)candy_pack_delete(pack);
  EXPECT_EQ((uint64_t)pack, (uint64_t)NULL);
}

TEST(pack, integer){
  candy_pack_t pack = candy_pack_integer(0, 114514);
  EXPECT_EQ(candy_pack_get_integer(pack), 114514);
  pack = candy_pack_set_integer(pack, 1919810);
  EXPECT_EQ(candy_pack_get_integer(pack), 1919810);
  EXPECT_FLOAT_EQ(candy_pack_get_float(pack), 1919810.0f);
  pack = (candy_pack_t)candy_pack_delete(pack);
  EXPECT_EQ((uint64_t)pack, (uint64_t)NULL);
}

TEST(pack, float){
  candy_pack_t pack = candy_pack_float(0, 6.626f);
  EXPECT_FLOAT_EQ(candy_pack_get_float(pack), 6.626f);
  EXPECT_EQ(candy_pack_get_integer(pack), 7);
  pack = candy_pack_set_float(pack, 3.1415926f);
  EXPECT_FLOAT_EQ(candy_pack_get_float(pack), 3.1415926f);
  EXPECT_EQ(candy_pack_get_integer(pack), 3);
  pack = (candy_pack_t)candy_pack_delete(pack);
  EXPECT_EQ((uint64_t)pack, (uint64_t)NULL);
}

TEST(pack, method){
  candy_pack_t pack = candy_pack_method(0, candy_method_print);
  candy_object_t obj = candy_object_create();
  EXPECT_EQ((uint64_t)candy_pack_get_method(pack), (uint64_t)candy_method_print);
  candy_pack_get_method(pack)(obj);
  pack = (candy_pack_t)candy_pack_delete(pack);
  obj = candy_object_delete(obj);
  EXPECT_EQ((uint64_t)pack, (uint64_t)NULL);
}

TEST(pack, object){
  candy_pack_t pack = candy_pack_object(0, candy_object_create());
  candy_object_t obj = candy_pack_get_object(pack);
  candy_object_push_method(obj, 1, candy_method_print);
  candy_object_get_method(obj, 1)(obj);
  pack = (candy_pack_t)candy_pack_delete(pack);
  EXPECT_EQ((uint64_t)pack, (uint64_t)NULL);
}

TEST(pack, recursive){
#define depth 1024
  candy_object_t obj[depth] = {0};
  for (int i = 0; i < depth; i++){
    obj[i] = candy_object_create();
    i != 0 ? candy_object_push_object(obj[i - 1], 0, obj[i]) : 0;
  }
  candy_pack_t pack = candy_pack_object(0, obj[0]);
  pack = (candy_pack_t)candy_pack_delete(pack);
  EXPECT_EQ((uint64_t)pack, (uint64_t)NULL);
#undef depth
}

TEST(pack, cast){
  uint16_t size = 0;
  candy_pack_t pack = candy_pack_none(0);
  pack = candy_pack_set_string(pack, (char *)"hello world", sizeof("hello world"));
  EXPECT_STREQ(candy_pack_get_string(pack, &size), (char *)"hello world");
  pack = candy_pack_set_integer(pack, 114514);
  EXPECT_EQ(candy_pack_get_integer(pack), 114514);
  pack = candy_pack_set_float(pack, 3.1415926f);
  EXPECT_FLOAT_EQ(candy_pack_get_float(pack), 3.1415926f);
  pack = candy_pack_set_method(pack, candy_method_print);
  EXPECT_EQ((uint64_t)candy_pack_get_method(pack), (uint64_t)candy_method_print);
  pack = (candy_pack_t)candy_pack_delete(pack);
  EXPECT_EQ((uint64_t)pack, (uint64_t)NULL);
}
