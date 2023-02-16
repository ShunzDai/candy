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
#include "src/candy_object.h"

TEST(object, lifecycle) {
  candy_object_t *self = candy_object_create("");
  candy_object_print(self);
  candy_object_delete(&self);
  EXPECT_EQ((uint64_t)self, (uint64_t)NULL);
}

TEST(object, recursive) {
#define depth 4
  candy_object_t *self = candy_object_create("");
  for (int i = 0; i < depth; i++) {
    candy_object_t *obj = candy_object_create("");
    candy_float_t flt = 3.1415926;
    candy_object_add_float(obj, "", &flt, 1);
    candy_object_add_string(obj, "", "hello world", sizeof("hello world"));
    candy_object_add_object(obj, self);
    self = obj;
  }
  candy_object_print(self);
  candy_object_delete(&self);
  EXPECT_EQ((uint64_t)self, (uint64_t)NULL);
#undef depth
}
