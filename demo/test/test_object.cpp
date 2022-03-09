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
#include "src/struct/candy_object.h"
#include "src/method/candy_standard.h"

TEST(object, lifecycle){
  candy_object_t obj = candy_object_create();
  obj = candy_object_delete(obj);
  EXPECT_EQ((uint64_t)obj, (uint64_t)NULL);
}

TEST(object, method){
  candy_object_t obj = candy_object_create();
  candy_object_push_method(obj, (char *)"print", candy_method_print);
  candy_object_get_method(obj, (char *)"print")(obj);
  obj = candy_object_delete(obj);
  EXPECT_EQ((uint64_t)obj, (uint64_t)NULL);
}

