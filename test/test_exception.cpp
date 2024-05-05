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
#include "test.h"
#include "core/candy_exception.h"
#include "core/candy_gc.h"
#include "core/candy_array.h"

TEST(catch, exception_ok) {
  candy_exce_t jmp{};
  candy_array_t *err = candy_exce_try(&jmp, (candy_exce_cb_t)+[](void *arg) {
    EXPECT_EQ((uint64_t)arg, 0x12345678);
  }, (void *)0x12345678);
  EXPECT_EQ(err, nullptr);
}

TEST(catch, exception_err) {
  struct arg {
    candy_exce_t jmp;
    candy_gc_t gc;
  };
  arg info{};
  candy_gc_init(&info.gc, test_allocator, nullptr);
  candy_array_t *err = candy_exce_try(&info.jmp, (candy_exce_cb_t)+[](arg *info) {
    candy_exce_throw(&info->jmp, &info->gc, "assert string");
  }, &info);
  EXPECT_EQ(!err, false);
  EXPECT_MEMEQ(candy_array_data(err), "assert string", sizeof("assert string"));
  candy_handler_t list[CANDY_TYPE_NUM];
  list[CANDY_TYPE_CHAR] = (candy_handler_t)candy_array_delete;
  candy_gc_deinit(&info.gc, list);
}

TEST(catch, nest_ok) {
  candy_exce_t jmp{};
  candy_array_t *err = candy_exce_try(&jmp, (candy_exce_cb_t)+[](candy_exce_t *jmp) {
    candy_array_t *err = candy_exce_try(jmp, (candy_exce_cb_t)+[](candy_exce_t *jmp) {

    }, jmp);
    EXPECT_EQ(err, nullptr);
  }, &jmp);
  EXPECT_EQ(err, nullptr);
}

TEST(catch, nest_err) {
  struct arg {
    candy_exce_t jmp;
    candy_gc_t gc;
    size_t depth;
  };
  arg info{};
  candy_gc_init(&info.gc, test_allocator, nullptr);
  candy_array_t *err = candy_exce_try(&info.jmp, (candy_exce_cb_t)+[](arg *info) {
    EXPECT_EQ(++info->depth, candy_exce_depth(&info->jmp));
    candy_array_t *err = candy_exce_try(&info->jmp, (candy_exce_cb_t)+[](arg *info) {
      candy_exce_throw(&info->jmp, &info->gc, "depth %zu", candy_exce_depth(&info->jmp));
    }, info);
    EXPECT_EQ(!err, false);
    EXPECT_MEMEQ(candy_array_data(err), "depth 2", sizeof("depth 2"));
    candy_exce_throw(&info->jmp, &info->gc, "depth %zu", candy_exce_depth(&info->jmp));
  }, &info);
  EXPECT_EQ(!err, false);
  EXPECT_MEMEQ(candy_array_data(err), "depth 1", sizeof("depth 1"));
  candy_handler_t list[CANDY_TYPE_NUM];
  list[CANDY_TYPE_CHAR] = (candy_handler_t)candy_array_delete;
  candy_gc_deinit(&info.gc, list);
}
