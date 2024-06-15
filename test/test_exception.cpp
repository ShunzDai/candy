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
#include "core/candy_print.h"

static int handler(candy_object_t *self, candy_gc_t *gc, candy_events_t evt) {
  return candy_array_delete((candy_array_t *)self, gc);
}

TEST(catch, exception_ok) {
  candy_exce_t jmp{};
  auto err = candy_exce_try(&jmp, (candy_exce_cb_t)+[](void *arg) {
    EXPECT_EQ((uint64_t)arg, 0x12345678);
  }, (void *)0x12345678, NULL);
  EXPECT_EQ(err, EXCE_OK);
}

TEST(catch, exception_err) {
  struct arg {
    candy_exce_t jmp;
    candy_gc_t gc;
  };
  arg info{};
  candy_gc_init(&info.gc, handler, test_allocator, nullptr);
  candy_object_t *msg = nullptr;
  auto err = candy_exce_try(&info.jmp, (candy_exce_cb_t)+[](arg *info) {
    candy_exce_throw(&info->jmp, EXCE_ERR_LEXICAL, (candy_object_t *)candy_print(&info->gc, "assert string"));
  }, &info, &msg);
  EXPECT_EQ(err, EXCE_ERR_LEXICAL);
  EXPECT_MEMEQ(candy_array_data((candy_array_t *)msg), "assert string", sizeof("assert string"));
  candy_gc_deinit(&info.gc);
}

TEST(catch, nest_ok) {
  candy_exce_t jmp{};
  auto err = candy_exce_try(&jmp, (candy_exce_cb_t)+[](candy_exce_t *jmp) {
    auto err = candy_exce_try(jmp, (candy_exce_cb_t)+[](candy_exce_t *jmp) {

    }, jmp, nullptr);
    EXPECT_EQ(err, EXCE_OK);
  }, &jmp, nullptr);
  EXPECT_EQ(err, EXCE_OK);
}

TEST(catch, nest_err) {
  struct arg {
    candy_exce_t jmp;
    candy_gc_t gc;
    size_t depth;
  };
  arg info{};
  candy_gc_init(&info.gc, handler, test_allocator, nullptr);
  candy_object_t *msg = nullptr;
  auto err = candy_exce_try(&info.jmp, (candy_exce_cb_t)+[](arg *info) {
    EXPECT_EQ(++info->depth, candy_exce_depth(&info->jmp));
    candy_object_t *msg = nullptr;
    auto err = candy_exce_try(&info->jmp, (candy_exce_cb_t)+[](arg *info) {
      candy_exce_throw(&info->jmp, EXCE_ERR_LEXICAL,
        (candy_object_t *)candy_print(&info->gc, "depth %zu", candy_exce_depth(&info->jmp))
      );
    }, info, &msg);
    EXPECT_EQ(err, EXCE_ERR_LEXICAL);
    EXPECT_MEMEQ(candy_array_data((candy_array_t *)msg), "depth 2", sizeof("depth 2"));
    candy_exce_throw(&info->jmp, EXCE_ERR_SYNTAX,
      (candy_object_t *)candy_print(&info->gc, "depth %zu", candy_exce_depth(&info->jmp))
    );
  }, &info, &msg);
  EXPECT_EQ(err, EXCE_ERR_SYNTAX);
  EXPECT_MEMEQ(candy_array_data((candy_array_t *)msg), "depth 1", sizeof("depth 1"));
  candy_gc_deinit(&info.gc);
}
