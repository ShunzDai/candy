/**
  * Copyright 2022-2024 ShunzDai
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
#include "candy.h"

TEST(cpp, void_res_void_arg) {
  candy::state sta;
  sta.add(std::tuple{"func", +[](void) -> void {
  }});
  auto res = sta.call("func");
  EXPECT_EQ(std::tuple_size<decltype(res)>::value, 0);
}

TEST(cpp, void_res_single_arg) {
  candy::state sta;
  sta.add(std::tuple{"func", +[](int arg) -> void {
    EXPECT_EQ(arg, 114514);
  }});
  auto res = sta.call("func", 114514);
  EXPECT_EQ(std::tuple_size<decltype(res)>::value, 0);
}

TEST(cpp, single_res_void_arg) {
  candy::state sta;
  sta.add(std::tuple{"func", +[](void) -> int {
    return 114514;
  }});
  auto res = sta.call<int>("func");
  EXPECT_EQ(std::tuple_size<decltype(res)>::value, 1);
  EXPECT_EQ(std::get<0>(res), 114514);
}

TEST(cpp, single_res_single_arg) {
  candy::state sta;
  sta.add(std::tuple{"func", +[](int arg) -> int {
    EXPECT_EQ(arg, 114514);
    return arg;
  }});
  auto res = sta.call<int>("func", 114514);
  EXPECT_EQ(std::tuple_size<decltype(res)>::value, 1);
  EXPECT_EQ(std::get<0>(res), 114514);
}

TEST(cpp, void_res_muti_arg) {
  candy::state sta;
  sta.add(std::tuple{"func", +[](int arg1, float arg2) -> void {
    EXPECT_EQ(arg1, 114514);
    EXPECT_FLOAT_EQ(arg2, 3.14f);
  }});
  auto res = sta.call("func", 114514, 3.14f);
  EXPECT_EQ(std::tuple_size<decltype(res)>::value, 0);
}

TEST(cpp, muti_res_void_arg) {
  candy::state sta;
  sta.add(std::tuple{"func", +[]() -> std::tuple<int ,float> {
    return {114514, 3.14f};
  }});
  auto res = sta.call<int, float>("func");
  EXPECT_EQ(std::tuple_size<decltype(res)>::value, 2);
  EXPECT_EQ(std::get<0>(res), 114514);
  EXPECT_FLOAT_EQ(std::get<1>(res), 3.14f);
}

TEST(cpp, muti_res_muti_arg) {
  candy::state sta;
  sta.add(std::tuple{"func", +[](int arg1, float arg2) -> std::tuple<int ,float> {
    EXPECT_EQ(arg1, 114514);
    EXPECT_FLOAT_EQ(arg2, 3.14f);
    return {114514, 3.14f};
  }});
  auto res = sta.call<int, float>("func", 114514, 3.14f);
  EXPECT_EQ(std::tuple_size<decltype(res)>::value, 2);
  EXPECT_EQ(std::get<0>(res), 114514);
  EXPECT_FLOAT_EQ(std::get<1>(res), 3.14f);
}
