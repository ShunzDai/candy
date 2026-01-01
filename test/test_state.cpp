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

TEST(state, push_pull) {
  candy_state_t *self = candy_new_state_default();
  candy_state_push_integer(self, 111);
  EXPECT_EQ(candy_state_to_integer(self, 0), 111);
  EXPECT_EQ(candy_state_to_integer(self, -1), 111);
  candy_close(self);
}

TEST(state, call_0) {
  candy_state_t *self = candy_new_state_default();
  candy_state_push_cfunc(self, +[](candy_state_t *self) -> int {
    return 0;
  });
  EXPECT_EQ(candy_state_call(self, 0, 0), CANDY_OK);
  candy_close(self);
}

TEST(state, call_1) {
  candy_state_t *self = candy_new_state_default();
  candy_state_push_cfunc(self, +[](candy_state_t *self) -> int {
    EXPECT_EQ(candy_state_to_integer(self, 0), 111);
    EXPECT_EQ(candy_state_to_integer(self, -1), 111);
    return 0;
  });
  candy_state_push_integer(self, 111);
  EXPECT_EQ(candy_state_to_integer(self, 1), 111);
  EXPECT_EQ(candy_state_to_integer(self, -1), 111);
  EXPECT_EQ(candy_state_call(self, 1, 0), CANDY_OK);
  candy_close(self);
}

TEST(state, call_2) {
  candy_state_t *self = candy_new_state_default();
  candy_state_push_cfunc(self, +[](candy_state_t *self) -> int {
    candy_state_push_integer(self, 111);
    return 1;
  });
  EXPECT_EQ(candy_state_call(self, 0, 1), CANDY_OK);
  EXPECT_EQ(candy_state_to_integer(self, 0), 111);
  EXPECT_EQ(candy_state_to_integer(self, -1), 111);
  candy_close(self);
}

TEST(state, call_3) {
  candy_state_t *self = candy_new_state_default();
  candy_state_push_cfunc(self, +[](candy_state_t *self) -> int {
    EXPECT_EQ(candy_state_to_integer(self, 0), 111);
    EXPECT_EQ(candy_state_to_integer(self, -1), 111);
    candy_state_push_integer(self, 222);
    EXPECT_EQ(candy_state_to_integer(self, 1), 222);
    EXPECT_EQ(candy_state_to_integer(self, -1), 222);
    return 1;
  });
  candy_state_push_integer(self, 111);
  EXPECT_EQ(candy_state_call(self, 1, 1), CANDY_OK);
  EXPECT_EQ(candy_state_to_integer(self, 0), 222);
  EXPECT_EQ(candy_state_to_integer(self, -1), 222);
  candy_close(self);
}

TEST(state, call_4) {
  candy_state_t *self = candy_new_state_default();
  candy_state_push_cfunc(self, +[](candy_state_t *self) -> int {
    EXPECT_EQ(candy_state_to_integer(self, 0), 111);
    EXPECT_EQ(candy_state_to_integer(self, -1), 111);
    candy_state_push_integer(self, 222);
    EXPECT_EQ(candy_state_to_integer(self, 1), 222);
    EXPECT_EQ(candy_state_to_integer(self, -1), 222);
    candy_state_push_integer(self, 333);
    EXPECT_EQ(candy_state_to_integer(self, 1), 222);
    EXPECT_EQ(candy_state_to_integer(self, 2), 333);
    EXPECT_EQ(candy_state_to_integer(self, -2), 222);
    EXPECT_EQ(candy_state_to_integer(self, -1), 333);
    return 2;
  });
  candy_state_push_integer(self, 111);
  EXPECT_EQ(candy_state_call(self, 1, 2), CANDY_OK);
  EXPECT_EQ(candy_state_to_integer(self, 0), 222);
  EXPECT_EQ(candy_state_to_integer(self, 1), 333);
  EXPECT_EQ(candy_state_to_integer(self, -2), 222);
  EXPECT_EQ(candy_state_to_integer(self, -1), 333);
  candy_close(self);
}

TEST(state, call_nest_0) {
  candy_state_t *self = candy_new_state_default();
  candy_state_push_cfunc(self, +[](candy_state_t *self) -> int {
    EXPECT_EQ(candy_state_to_integer(self, 0), 111);
    EXPECT_EQ(candy_state_to_integer(self, -1), 111);
    /* nest call begin */
    candy_state_push_cfunc(self, +[](candy_state_t *self) -> int {
      EXPECT_EQ(candy_state_to_integer(self, 0), 222);
      EXPECT_EQ(candy_state_to_integer(self, -1), 222);
      candy_state_push_integer(self, 333);
      EXPECT_EQ(candy_state_to_integer(self, 1), 333);
      EXPECT_EQ(candy_state_to_integer(self, -1), 333);
      return 1;
    });
    candy_state_push_integer(self, 222);
    EXPECT_EQ(candy_state_to_integer(self, 2), 222);
    EXPECT_EQ(candy_state_to_integer(self, -1), 222);
    EXPECT_EQ(candy_state_call(self, 1, 1), CANDY_OK);
    /* nest call end */
    EXPECT_EQ(candy_state_to_integer(self, 1), 333);
    EXPECT_EQ(candy_state_to_integer(self, -1), 333);
    return 1;
  });
  candy_state_push_integer(self, 111);
  EXPECT_EQ(candy_state_to_integer(self, 1), 111);
  EXPECT_EQ(candy_state_to_integer(self, -1), 111);
  EXPECT_EQ(candy_state_call(self, 1, 1), CANDY_OK);
  EXPECT_EQ(candy_state_to_integer(self, 0), 333);
  EXPECT_EQ(candy_state_to_integer(self, -1), 333);
  candy_close(self);
}

TEST(state, call_nest_1) {
  candy_state_t *self = candy_new_state_default();
  candy_state_push_cfunc(self, +[](candy_state_t *self) -> int {
    EXPECT_EQ(candy_state_to_integer(self, 0), 111);
    EXPECT_EQ(candy_state_to_integer(self, 1), 222);
    EXPECT_EQ(candy_state_to_integer(self, -2), 111);
    EXPECT_EQ(candy_state_to_integer(self, -1), 222);
    /* nest call begin */
    candy_state_push_cfunc(self, +[](candy_state_t *self) -> int {
      EXPECT_EQ(candy_state_to_integer(self, 0), 333);
      EXPECT_EQ(candy_state_to_integer(self, 1), 444);
      EXPECT_EQ(candy_state_to_integer(self, 2), 555);
      EXPECT_EQ(candy_state_to_integer(self, -3), 333);
      EXPECT_EQ(candy_state_to_integer(self, -2), 444);
      EXPECT_EQ(candy_state_to_integer(self, -1), 555);
      candy_state_push_integer(self, 666);
      EXPECT_EQ(candy_state_to_integer(self, 3), 666);
      EXPECT_EQ(candy_state_to_integer(self, -1), 666);
      return 1;
    });
    candy_state_push_integer(self, 333);
    candy_state_push_integer(self, 444);
    candy_state_push_integer(self, 555);
    EXPECT_EQ(candy_state_to_integer(self, 3), 333);
    EXPECT_EQ(candy_state_to_integer(self, 4), 444);
    EXPECT_EQ(candy_state_to_integer(self, 5), 555);
    EXPECT_EQ(candy_state_to_integer(self, -3), 333);
    EXPECT_EQ(candy_state_to_integer(self, -2), 444);
    EXPECT_EQ(candy_state_to_integer(self, -1), 555);
    EXPECT_EQ(candy_state_call(self, 3, 1), CANDY_OK);
    /* nest call end */
    EXPECT_EQ(candy_state_to_integer(self, 2), 666);
    EXPECT_EQ(candy_state_to_integer(self, -1), 666);
    candy_state_push_integer(self, 777);
    candy_state_push_integer(self, 888);
    EXPECT_EQ(candy_state_to_integer(self, 3), 777);
    EXPECT_EQ(candy_state_to_integer(self, 4), 888);
    EXPECT_EQ(candy_state_to_integer(self, -2), 777);
    EXPECT_EQ(candy_state_to_integer(self, -1), 888);
    return 3;
  });
  candy_state_push_integer(self, 111);
  candy_state_push_integer(self, 222);
  EXPECT_EQ(candy_state_to_integer(self, 1), 111);
  EXPECT_EQ(candy_state_to_integer(self, 2), 222);
  EXPECT_EQ(candy_state_to_integer(self, -2), 111);
  EXPECT_EQ(candy_state_to_integer(self, -1), 222);
  EXPECT_EQ(candy_state_call(self, 2, 3), CANDY_OK);
  EXPECT_EQ(candy_state_to_integer(self, 0), 666);
  EXPECT_EQ(candy_state_to_integer(self, 1), 777);
  EXPECT_EQ(candy_state_to_integer(self, 2), 888);
  EXPECT_EQ(candy_state_to_integer(self, -3), 666);
  EXPECT_EQ(candy_state_to_integer(self, -2), 777);
  EXPECT_EQ(candy_state_to_integer(self, -1), 888);
  candy_close(self);
}

TEST(state, call_nest_2) {
  candy_state_t *self = candy_new_state_default();
  candy_state_push_cfunc(self, +[](candy_state_t *self) -> int {
    EXPECT_EQ(candy_state_to_integer(self, 0), 111);
    EXPECT_EQ(candy_state_to_integer(self, -1), 111);
    /* nest call begin */
    candy_state_push_cfunc(self, +[](candy_state_t *self) -> int {
      EXPECT_EQ(candy_state_to_integer(self, 0), 222);
      EXPECT_EQ(candy_state_to_integer(self, -1), 222);
      /* nest call begin */
      candy_state_push_cfunc(self, +[](candy_state_t *self) -> int {
        EXPECT_EQ(candy_state_to_integer(self, 0), 333);
        EXPECT_EQ(candy_state_to_integer(self, -1), 333);
        candy_state_push_integer(self, 444);
        EXPECT_EQ(candy_state_to_integer(self, 1), 444);
        EXPECT_EQ(candy_state_to_integer(self, -1), 444);
        return 1;
      });
      candy_state_push_integer(self, 333);
      EXPECT_EQ(candy_state_to_integer(self, 2), 333);
      EXPECT_EQ(candy_state_to_integer(self, -1), 333);
      EXPECT_EQ(candy_state_call(self, 1, 1), CANDY_OK);
      /* nest call end */
      EXPECT_EQ(candy_state_to_integer(self, 1), 444);
      EXPECT_EQ(candy_state_to_integer(self, -1), 444);
      return 1;
    });
    candy_state_push_integer(self, 222);
    EXPECT_EQ(candy_state_call(self, 1, 1), CANDY_OK);
    /* nest call end */
    EXPECT_EQ(candy_state_to_integer(self, 1), 444);
    EXPECT_EQ(candy_state_to_integer(self, -1), 444);
    return 1;
  });
  candy_state_push_integer(self, 111);
  EXPECT_EQ(candy_state_to_integer(self, 1), 111);
  EXPECT_EQ(candy_state_to_integer(self, -1), 111);
  EXPECT_EQ(candy_state_call(self, 1, 1), CANDY_OK);
  EXPECT_EQ(candy_state_to_integer(self, 0), 444);
  EXPECT_EQ(candy_state_to_integer(self, -1), 444);
  candy_close(self);
}

TEST(state, call_loop_0) {
  candy_state_t *self = candy_new_state_default();
  for (int i = 0; i < 10; ++i) {
    candy_state_push_cfunc(self, +[](candy_state_t *self) -> int {
      EXPECT_EQ(candy_state_to_integer(self, 0), 111);
      EXPECT_EQ(candy_state_to_integer(self, -1), 111);
      candy_state_push_integer(self, 222);
      EXPECT_EQ(candy_state_to_integer(self, 1), 222);
      EXPECT_EQ(candy_state_to_integer(self, -1), 222);
      return 1;
    });
    candy_state_push_integer(self, 111);
    EXPECT_EQ(candy_state_call(self, 1, 1), CANDY_OK);
    EXPECT_EQ(candy_state_to_integer(self, i), 222);
    EXPECT_EQ(candy_state_to_integer(self, -1), 222);
  }
  candy_close(self);
}
