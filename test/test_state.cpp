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

TEST(state, gc_preserves_globals_across_scripts) {
  static int checks;
  checks = 0;
  candy_state_t *self = candy_new_state_default();
  ASSERT_NE(self, nullptr);
  ASSERT_EQ(candy_state_setglobal(self, "verify", +[](candy_state_t *state) -> int {
    ++checks;
    EXPECT_EQ(candy_state_to_integer(state, 0), 55);
    return 0;
  }), CANDY_OK);
  const char definition[] = R"(def fibonacci(n)
if (n < 2)
return n
end
return fibonacci(n - 2) + fibonacci(n - 1)
end
)";
  ASSERT_EQ(candy_dostring(self, definition, sizeof(definition) - 1), CANDY_OK);
  const char call[] = R"(verify(fibonacci(10))
)";
  for (int iteration = 0; iteration < 3; ++iteration) {
    ASSERT_EQ(candy_dostring(self, call, sizeof(call) - 1), CANDY_OK);
    EXPECT_EQ(checks, iteration + 1);
  }
  candy_close(self);
}

TEST(state, gc_preserves_objects_on_stack) {
  candy_state_t *self = candy_new_state_default();
  ASSERT_NE(self, nullptr);
  const char first[] = R"(return "retained")";
  ASSERT_EQ(candy_dostring(self, first, sizeof(first) - 1), CANDY_OK);
  ASSERT_EQ(candy_state_get_type(self, 0), CANDY_TYPE_CHAR);
  candy_object_t *retained = candy_state_to_object(self, 0);
  const char next[] = R"(return 123)";
  for (int cycle = 0; cycle < 8; ++cycle) {
    ASSERT_EQ(candy_dostring(self, next, sizeof(next) - 1), CANDY_OK);
    EXPECT_EQ(candy_state_to_object(self, 0), retained);
    auto *text = reinterpret_cast<candy_array_t *>(retained);
    EXPECT_EQ(candy_array_size(text), 8U);
    EXPECT_MEMEQ(candy_array_data(text), "retained", 8);
    EXPECT_EQ(candy_state_to_integer(self, -1), 123);
  }
  candy_close(self);
}

TEST(state, vm_loop_collects_garbage_and_preserves_active_roots) {
  struct statistics {
    candy_gc_t *collector;
    size_t collections;
    size_t deleted;
    size_t peak;
    int allocations;
    int checks;
    bool fail;
  };
  static statistics stats;
  stats = {};
  auto handler = +[](candy_object_t *object, candy_gc_t *collector,
                     candy_events_t event, void *arg) -> candy_err_t {
    stats.collector = collector;
    if (stats.fail && event == EVT_COLOR)
      return CANDY_ERR;
    if (event == EVT_COLOR && candy_object_type(object) == CANDY_TYPE_STATE)
      ++stats.collections;
    if (event == EVT_DELETE && (candy_object_mask(object) & MASK_ARRAY))
      ++stats.deleted;
    if (candy_object_mask(object) & MASK_ARRAY)
      return candy_array_handler(reinterpret_cast<candy_array_t *>(object), collector, event, arg);
    switch (candy_object_type(object)) {
      case CANDY_TYPE_TABLE:
        return candy_table_handler(reinterpret_cast<candy_table_t *>(object), collector, event, arg);
      case CANDY_TYPE_PROTO:
        return candy_proto_handler(reinterpret_cast<candy_proto_t *>(object), collector, event, arg);
      case CANDY_TYPE_SCLOS:
        return candy_sclosure_handler(reinterpret_cast<candy_sclosure_t *>(object), collector, event, arg);
      case CANDY_TYPE_CCLOS:
        return candy_cclosure_handler(reinterpret_cast<candy_cclosure_t *>(object), collector, event, arg);
      case CANDY_TYPE_STATE:
        return candy_state_handler(reinterpret_cast<candy_state_t *>(object), collector, event, arg);
      default:
        return CANDY_ERR;
    }
  };
  candy_state_t *self = candy_state_create(handler, test_allocator, nullptr);
  ASSERT_NE(self, nullptr);
  ASSERT_EQ(candy_state_setglobal(self, "make", +[](candy_state_t *state) -> int {
    auto *array = candy_array_create(stats.collector, nullptr, CANDY_TYPE_CHAR);
    candy_array_resize(array, stats.collector, nullptr, 8192);
    memcpy(candy_array_data(array), "alive", 5);
    EXPECT_EQ(candy_state_push_object(state, reinterpret_cast<candy_object_t *>(array)), CANDY_OK);
    return 1;
  }), CANDY_OK);
  ASSERT_EQ(candy_state_setglobal(self, "allocate", +[](candy_state_t *) -> int {
    stats.collector->threshold = 64 * 1024;
    auto *garbage = candy_array_create(stats.collector, nullptr, CANDY_TYPE_CHAR);
    candy_array_resize(garbage, stats.collector, nullptr, 8192);
    stats.peak = std::max(stats.peak, candy_memory_used(candy_gc_memory(stats.collector)));
    ++stats.allocations;
    return 0;
  }), CANDY_OK);
  ASSERT_EQ(candy_state_setglobal(self, "verify", +[](candy_state_t *state) -> int {
    ++stats.checks;
    EXPECT_GT(stats.collections, 0U);
    EXPECT_GT(stats.deleted, 0U);
    EXPECT_LT(stats.peak, 128U * 1024);
    EXPECT_EQ(stats.allocations, 24);
    EXPECT_EQ(candy_state_get_type(state, 0), CANDY_TYPE_CHAR);
    auto *array = reinterpret_cast<candy_array_t *>(candy_state_to_object(state, 0));
    EXPECT_EQ(candy_array_size(array), 8192U);
    EXPECT_MEMEQ(candy_array_data(array), "alive", 5);
    return 0;
  }), CANDY_OK);
  const char source[] = R"(def churn(n, held)
if (n < 1)
verify(held)
return held
end
allocate()
return churn(n - 1, held)
end
return churn(24, make())
)";
  EXPECT_EQ(candy_dostring(self, source, sizeof(source) - 1), CANDY_OK);
  EXPECT_EQ(stats.checks, 1);
  ASSERT_EQ(candy_state_setglobal(self, "fail_gc", +[](candy_state_t *) -> int {
    stats.collector->threshold = 0;
    stats.fail = true;
    return 0;
  }), CANDY_OK);
  const char failure[] = R"(def fail_nested(n)
if (n < 1)
fail_gc()
return 123
end
return fail_nested(n - 1)
end
return fail_nested(4)
)";
  EXPECT_EQ(candy_dostring(self, failure, sizeof(failure) - 1), CANDY_ERR_VM);
  stats.fail = false;
  const char recovery[] = R"(return 123)";
  EXPECT_EQ(candy_dostring(self, recovery, sizeof(recovery) - 1), CANDY_OK);
  EXPECT_EQ(candy_state_to_integer(self, -1), 123);
  EXPECT_EQ(candy_close(self), CANDY_OK);
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
