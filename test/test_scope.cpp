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

class scope : public testing::Test {
protected:
  candy_state_t *state = nullptr;

  void SetUp() override {
    state = candy_new_state_default();
    ASSERT_NE(state, nullptr);
  }

  void TearDown() override {
    if (state) {
      EXPECT_EQ(candy_close(state), CANDY_OK);
    }
  }

  void integer(const char *source, candy_integer_t expected) {
    SCOPED_TRACE(source);
    ASSERT_EQ(candy_dostring(state, source, strlen(source)), CANDY_OK);
    ASSERT_EQ(candy_state_get_type(state, -1), CANDY_TYPE_INTEGER);
    EXPECT_EQ(candy_state_to_integer(state, -1), expected);
  }

  void missing_global(const char *source) {
    SCOPED_TRACE(source);
    ASSERT_EQ(candy_dostring(state, source, strlen(source)), CANDY_OK);
    EXPECT_EQ(candy_state_get_type(state, -1), CANDY_TYPE_NULL);
  }

  void none(const char *source) {
    SCOPED_TRACE(source);
    ASSERT_EQ(candy_dostring(state, source, strlen(source)), CANDY_OK);
    EXPECT_EQ(candy_state_get_type(state, -1), CANDY_TYPE_NONE);
  }
};

TEST_F(scope, top_level_assignments_and_definitions_remain_global) {
  integer(R"(value = 40
def increment(n)
  return n + 1
end
value = increment(value)
return value
)", 41);
  integer("value = increment(value)\nreturn value", 42);
}

TEST_F(scope, assignment_declares_local_after_evaluating_rhs) {
  integer(R"(value = 40
def local_value()
  before = value
  value = value + 1
  value = value + 1
  return before + value
end
return local_value()
)", 82);
  integer("return value", 40);
  missing_global("return before");
}

TEST_F(scope, new_locals_do_not_leak_or_persist_between_calls) {
  integer(R"(def local_value(n)
  fresh = n + 1
  return fresh
end
return local_value(10) + local_value(20)
)", 32);
  missing_global("return fresh");
  missing_global("return n");
}

TEST_F(scope, parameters_are_mutable_locals) {
  integer(R"(value = 90
def change(value)
  value = value + 1
  value = value + value
  return value
end
return change(4) + change(7)
)", 26);
  integer("return value", 90);
}

TEST_F(scope, augmented_local_adds_keep_value_and_non_augmented_forms) {
  integer(R"(def f()
  x = 0
  y = 5
  x = x + 1
  x = x + y
  x = x - 2
  y = x + y
  x = y + x
  return x
end
return f()
)", 13);
}

TEST_F(scope, while_reuses_local_slots_and_body_has_no_scope) {
  integer(R"(index = 90
def count()
  index = 0
  total = 0
  while (index < 1000)
    scratch = index + 1
    total = total + scratch
    index = index + 1
  end
  return total + scratch + index
end
return count()
)", 502500);
  integer("return index", 90);
  missing_global("return scratch");
  missing_global("return total");
}

TEST_F(scope, for_reuses_local_slots_and_clauses_share_function_scope) {
  integer(R"(index = 90
def count()
  total = 0
  index = 0
  while (index < 1000)
    scratch = index + 1
    total = total + scratch
    index = index + 1
  end
  return total + scratch + index
end
return count()
)", 502500);
  integer("return index", 90);
  missing_global("return scratch");
}

TEST_F(scope, if_does_not_introduce_a_scope) {
  integer(R"(value = 90
def choose(flag)
  if (flag)
    value = 7
  else
    value = 9
  end
  value = value + 1
  return value
end
return choose(true) + choose(false)
)", 18);
  integer("return value", 90);
}

TEST_F(scope, skipped_assignment_reserves_a_none_initialized_local) {
  none(R"(value = 90
def choose()
  if (false)
    value = 7
  end
  return value
end
return choose()
)");
  integer("return value", 90);
}

TEST_F(scope, skipped_loop_assignment_reserves_a_none_initialized_local) {
  none(R"(value = 90
def choose()
  while (false)
    value = 7
  end
  return value
end
return choose()
)");
  integer("return value", 90);
}

TEST_F(scope, local_slots_are_reset_to_none_on_each_entry) {
  none(R"(def choose(flag)
  if (flag)
    value = 7
  end
  return value
end
previous = choose(true)
return choose(false)
)");
  integer("return previous", 7);
  missing_global("return value");
}

TEST_F(scope, nested_definition_is_local_and_does_not_replace_global) {
  integer(R"(def helper(n)
  return n + 100
end
def outer(n)
  def helper(value)
    return value + 1
  end
  return helper(n)
end
return outer(4)
)", 5);
  integer("return helper(4)", 104);
}

TEST_F(scope, new_nested_definition_does_not_pollute_globals) {
  integer(R"(def outer()
  def inner(n)
    return n + 1
  end
  return inner(4)
end
return outer()
)", 5);
  missing_global("return inner");
}

TEST_F(scope, nested_function_recurses_through_its_own_self_binding) {
  integer(R"(def outer(n)
  def countdown(value)
    if (value < 1)
      return 0
    end
    return countdown(value - 1) + 1
  end
  return countdown(n)
end
return outer(12)
)", 12);
  missing_global("return countdown");
}

TEST_F(scope, returned_nested_function_keeps_recursive_self_without_upvalues) {
  integer(R"(def outer()
  def countdown(value)
    if (value < 1)
      return 0
    end
    return countdown(value - 1) + 1
  end
  return countdown
end
function = outer()
return function(12)
)", 12);
  missing_global("return countdown");
}

TEST_F(scope, function_parameter_can_be_called_and_reassigned) {
  integer(R"(def increment(n)
  return n + 1
end
def twice(n)
  return n + n
end
def apply(function, n)
  before = function(n)
  function = twice
  return before + function(n)
end
return apply(increment, 4)
)", 13);
  integer("return increment(4)", 5);
  missing_global("return function");
}

TEST_F(scope, local_function_binding_can_be_overwritten) {
  integer(R"(def replacement(n)
  return n + 10
end
def outer()
  def helper(n)
    return n + 1
  end
  before = helper(4)
  helper = replacement
  return before + helper(4)
end
return outer()
)", 19);
  missing_global("return helper");
}

TEST_F(scope, assignment_can_shadow_recursive_self_name) {
  integer(R"(def identity(value)
  identity = value + 1
  return identity
end
return identity(4) + identity(8)
)", 14);
  integer("return identity(10)", 11);
}

TEST_F(scope, nested_function_resolves_unshadowed_names_from_globals) {
  integer(R"(value = 40
def outer()
  def inner()
    return value + 1
  end
  return inner()
end
return outer()
)", 41);
  integer("value = 80\nreturn outer()", 81);
}

TEST_F(scope, callback_object_survives_collection_while_held_only_in_local) {
  struct statistics {
    candy_gc_t *collector;
    size_t collections;
    int checks;
  };
  static statistics stats;
  stats = {};
  auto handler = +[](candy_object_t *object, candy_gc_t *collector,
                     candy_events_t event, void *arg) -> candy_err_t {
    stats.collector = collector;
    if (event == EVT_COLOR && candy_object_type(object) == CANDY_TYPE_STATE)
      ++stats.collections;
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
  ASSERT_EQ(candy_close(state), CANDY_OK);
  state = candy_state_create(handler, test_allocator, nullptr);
  ASSERT_NE(state, nullptr);
  ASSERT_EQ(candy_state_setglobal(state, "make", +[](candy_state_t *current) -> int {
    auto *array = candy_array_create(stats.collector, nullptr, CANDY_TYPE_CHAR);
    candy_array_resize(array, stats.collector, nullptr, 8192);
    memcpy(candy_array_data(array), "alive", 5);
    EXPECT_EQ(candy_state_push_object(current, reinterpret_cast<candy_object_t *>(array)), CANDY_OK);
    return 1;
  }), CANDY_OK);
  ASSERT_EQ(candy_state_setglobal(state, "allocate", +[](candy_state_t *) -> int {
    auto *array = candy_array_create(stats.collector, nullptr, CANDY_TYPE_CHAR);
    candy_array_resize(array, stats.collector, nullptr, 8192);
    stats.collector->threshold = 0;
    return 0;
  }), CANDY_OK);
  ASSERT_EQ(candy_state_setglobal(state, "verify", +[](candy_state_t *current) -> int {
    ++stats.checks;
    EXPECT_GT(stats.collections, 0U);
    EXPECT_EQ(candy_state_get_type(current, 0), CANDY_TYPE_CHAR);
    if (candy_state_get_type(current, 0) == CANDY_TYPE_CHAR) {
      auto *array = reinterpret_cast<candy_array_t *>(candy_state_to_object(current, 0));
      EXPECT_EQ(candy_array_size(array), 8192U);
      EXPECT_MEMEQ(candy_array_data(array), "alive", 5);
    }
    return 0;
  }), CANDY_OK);
  integer(R"(def hold()
  held = make()
  index = 0
  while (index < 32)
    allocate()
    index = index + 1
  end
  verify(held)
  return index
end
return hold()
)", 32);
  EXPECT_EQ(stats.checks, 1);
  missing_global("return held");
  missing_global("return index");
}
