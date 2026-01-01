/**
  * Copyright 2022-2026 ShunzDai
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

TEST(catch, exception_ok) {
  candy_excep_t jmp{};
  EXPECT_EQ(candy_excep_depth(&jmp), 0);
  auto err = candy_excep_try(&jmp, (candy_excep_cb_t)+[](void *arg) {
    EXPECT_EQ((uint64_t)arg, 0x12345678);
  }, (void *)0x12345678, NULL);
  EXPECT_EQ(err, CANDY_OK);
}

TEST(catch, exception_err) {
  typedef struct arg {
    candy_excep_t jmp;
    candy_gc_t gc;
  } arg_t;
  arg_t info{};
  candy_gc_init(&info.gc, nullptr, (candy_handler_t)candy_array_handler, test_allocator, nullptr);
  candy_object_t *msg = nullptr;
  auto err = candy_excep_try(&info.jmp, +[](void *arg) {
    arg_t *info = (arg_t *)arg;
    candy_excep_throw(&info->jmp, CANDY_ERR_LEXICAL, (candy_object_t *)candy_array_print(&info->gc, nullptr, "assert string"));
  }, &info, &msg);
  EXPECT_EQ(err, CANDY_ERR_LEXICAL);
  EXPECT_MEMEQ(candy_array_data((candy_array_t *)msg), "assert string", sizeof("assert string"));
  candy_gc_deinit(&info.gc);
}

TEST(catch, nest_ok) {
  candy_excep_t jmp{};
  EXPECT_EQ(candy_excep_depth(&jmp), 0);
  auto err = candy_excep_try(&jmp, +[](void *arg) {
    candy_excep_t *jmp = (candy_excep_t *)arg;
    EXPECT_EQ(candy_excep_depth(jmp), 1);
    auto err = candy_excep_try(jmp, +[](void *arg) {
      candy_excep_t *jmp = (candy_excep_t *)arg;
      ASSERT_EQ(candy_excep_depth(jmp), 2);
      ASSERT_EQ(jmp != nullptr                  , true);
      ASSERT_EQ(jmp->prev != nullptr            , true);
      ASSERT_EQ(jmp->prev->prev != nullptr      , true);
      ASSERT_EQ(jmp->prev->prev->prev == nullptr, true);
    }, jmp, nullptr);
    EXPECT_EQ(err, CANDY_OK);
  }, &jmp, nullptr);
  EXPECT_EQ(err, CANDY_OK);
}

TEST(catch, nest_err) {
  struct arg {
    candy_excep_t jmp;
    candy_gc_t gc;
    size_t depth;
  };
  arg info{};
  candy_gc_init(&info.gc, nullptr, (candy_handler_t)candy_array_handler, test_allocator, nullptr);
  candy_object_t *msg = nullptr;
  auto err = candy_excep_try(&info.jmp, (candy_excep_cb_t)+[](arg *info) {
    EXPECT_EQ(++info->depth, candy_excep_depth(&info->jmp));
    candy_object_t *msg = nullptr;
    auto err = candy_excep_try(&info->jmp, (candy_excep_cb_t)+[](arg *info) {
      candy_excep_throw(&info->jmp, CANDY_ERR_LEXICAL,
        (candy_object_t *)candy_array_print(&info->gc, nullptr, "depth %zu", candy_excep_depth(&info->jmp))
      );
    }, info, &msg);
    EXPECT_EQ(err, CANDY_ERR_LEXICAL);
    EXPECT_MEMEQ(candy_array_data((candy_array_t *)msg), "depth 2", sizeof("depth 2"));
    candy_excep_throw(&info->jmp, CANDY_ERR_SYNTAX,
      (candy_object_t *)candy_array_print(&info->gc, nullptr, "depth %zu", candy_excep_depth(&info->jmp))
    );
  }, &info, &msg);
  EXPECT_EQ(err, CANDY_ERR_SYNTAX);
  EXPECT_MEMEQ(candy_array_data((candy_array_t *)msg), "depth 1", sizeof("depth 1"));
  candy_gc_deinit(&info.gc);
}
