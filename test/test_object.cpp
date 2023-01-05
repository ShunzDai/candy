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
#include "src/common/candy_lib.h"
#include "src/struct/candy_object.h"
#include "src/method/candy_standard.h"

TEST(object, lifecycle) {
  candy_object_t obj = candy_object_create(0);
  candy_object_delete(&obj);
  EXPECT_EQ((uint64_t)obj, (uint64_t)NULL);
}

TEST(object, recursive) {
#define depth 4
  candy_object_t obj[depth] = {0};
  for (int i = 0; i < depth; i++) {
    obj[i] = candy_object_create(0);
    candy_object_push_none(obj[i], 0);
    if (i != 0)
      candy_object_push(obj[i - 1], (candy_wrap_t)obj[i]);
  }
  candy_object_print(obj[0]);
  candy_object_delete(&obj[0]);
  EXPECT_EQ((uint64_t)obj[0], (uint64_t)NULL);
#undef depth
}

TEST(object, pop) {
  candy_object_t obj = candy_object_create(0);
  candy_object_push_integer(obj, 0, 114514);
  candy_object_push_float(obj, 1, 3.1415926f);
  candy_object_push_none(obj, 2);
  candy_object_print(obj);
  candy_object_pop(obj, 1);
  candy_object_print(obj);
  candy_object_delete(&obj);
  EXPECT_EQ((uint64_t)obj, (uint64_t)NULL);
}

TEST(obj, method) {
  candy_object_t obj = candy_object_create(0);
  candy_object_t param = candy_object_create(1);
  candy_object_push(obj, (candy_wrap_t)param);
  candy_object_push_method(obj, 2, candy_std_print);
  candy_object_push_string(param, 0, "hello world", strlen("hello world"));
  candy_object_push_integer(param, 0, 114514);
  candy_object_print(obj);
  param = candy_object_get_object(obj, 1);
  candy_method_t method = candy_object_get_method(obj, 2);
  EXPECT_EQ(!(uint64_t)method, (uint64_t)NULL);
  method(param);
  candy_object_delete(&obj);
  EXPECT_EQ((uint64_t)obj, (uint64_t)NULL);
}
