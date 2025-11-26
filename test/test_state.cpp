/**
  * Copyright 2022-2025 ShunzDai
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

TEST(state, push_pull) {
  candy_state_t *co = candy_new_state_default();
  candy_state_push_integer(co, 123);
  EXPECT_EQ(candy_state_to_integer(co, 0), 123);
  EXPECT_EQ(candy_state_to_integer(co, -1), 123);
  candy_close(co);
}

TEST(state, call) {
  candy_state_t *co = candy_new_state_default();
  candy_state_push_cfunc(co, +[](candy_state_t *self) -> int {
    EXPECT_EQ(candy_state_to_integer(self, 0), 123);
    EXPECT_EQ(candy_state_to_integer(self, -1), 123);
    candy_state_push_integer(self, 456);
    return 1;
  });
  candy_state_push_integer(co, 123);
  EXPECT_EQ(candy_state_call(co, 1, 1), CANDY_OK);
  printf(">>> line %d\n", __LINE__);
  EXPECT_EQ(candy_state_to_integer(co, 0), 456);
  printf(">>> line %d\n", __LINE__);
  EXPECT_EQ(candy_state_to_integer(co, -1), 456);
  printf(">>> line %d\n", __LINE__);
  candy_close(co);
}

TEST(state, call_nest) {
  candy_state_t *co = candy_new_state_default();
  candy_state_push_cfunc(co, +[](candy_state_t *self) -> int {
    EXPECT_EQ(candy_state_to_integer(self, 0), 123);
    EXPECT_EQ(candy_state_to_integer(self, -1), 123);
    candy_state_push_cfunc(self, +[](candy_state_t *self) -> int {
      EXPECT_EQ(candy_state_to_integer(self, 0), 456);
      EXPECT_EQ(candy_state_to_integer(self, -1), 456);
      candy_state_push_integer(self, 789);
      return 1;
    });
    candy_state_push_integer(self, 456);
    EXPECT_EQ(candy_state_call(self, 1, 1), CANDY_OK);
    EXPECT_EQ(candy_state_to_integer(self, 0), 789);
    EXPECT_EQ(candy_state_to_integer(self, -1), 789);
    return 1;
  });
  candy_state_push_integer(co, 123);
  EXPECT_EQ(candy_state_call(co, 1, 1), CANDY_OK);
  EXPECT_EQ(candy_state_to_integer(co, 0), 456);
  EXPECT_EQ(candy_state_to_integer(co, -1), 456);
  candy_close(co);
}
