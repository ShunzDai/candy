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
#include "test_common.h"
#include "src/candy_io.h"

TEST(io, exception_ok) {
  candy_io_t io;
  candy_io_init(&io);
  EXPECT_EQ(candy_io_try_catch(&io, +[](void *handle, void *ud) {
    EXPECT_EQ((uint64_t)handle, 1);
    EXPECT_EQ((uint64_t)ud, 2);
  }, (void *)1, (void *)2), 0);
  candy_io_deinit(&io);
}

TEST(io, exception_err) {
  candy_io_t io;
  candy_io_init(&io);
  EXPECT_EQ(candy_io_try_catch(&io, (candy_try_catch_cb_t)+[](candy_io_t *io, void *ud) {
    candy_io_throw(io, "assert string");
  }, &io, nullptr), -1);
  EXPECT_MEMEQ(candy_wrap_get_string(&io.buff), "assert string", sizeof("assert string"));
  candy_io_deinit(&io);
}

TEST(io, format) {
  candy_io_t io;
  candy_io_init(&io);
  EXPECT_EQ(candy_io_try_catch(&io, (candy_try_catch_cb_t)+[](candy_io_t *io, void *ud) {
    candy_io_throw(io, "%010d", 114514);
  }, &io, nullptr), -1);
  EXPECT_MEMEQ(candy_wrap_get_string(&io.buff), "0000114514", sizeof("0000114514"));
  candy_io_deinit(&io);
}
