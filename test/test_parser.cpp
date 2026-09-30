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

class parser : public testing::Test {
protected:
  candy_state_t *state = nullptr;
  inline static int checks = 0;

  void SetUp() override {
    state = candy_new_state_default();
    ASSERT_NE(state, nullptr);
    ASSERT_EQ(candy_state_setglobal(state, "verify", +[](candy_state_t *current) -> int {
      ++checks;
      EXPECT_EQ(candy_state_get_type(current, 0), CANDY_TYPE_INTEGER);
      if (candy_state_get_type(current, 0) == CANDY_TYPE_INTEGER) {
        EXPECT_EQ(candy_state_to_integer(current, 0), candy_state_to_integer(current, 1));
      }
      return 0;
    }), CANDY_OK);
  }

  void TearDown() override {
    if (state) {
      EXPECT_EQ(candy_close(state), CANDY_OK);
    }
  }

  void run(const char *source, int expected_checks) {
    checks = 0;
    ASSERT_EQ(candy_dostring(state, source, strlen(source)), CANDY_OK);
    EXPECT_EQ(checks, expected_checks);
  }
};

TEST_F(parser, arithmetic_associativity_and_parentheses) {
  run(R"(verify(1 + 2, 3)
verify(10 - 3 - 2, 5)
verify(10 - (3 - 2), 9)
verify(1 + 2 - 3 + 4, 4)
verify((1 + 2) + (3 - 4), 2)
verify(0 - 1, 0 - 1)
verify(0x10 + 2, 18)
)", 7);
}

TEST_F(parser, comparisons_respect_arithmetic_precedence) {
  run(R"(result = 0
if (4 < 2 + 1)
  result = 1
end
verify(result, 0)
result = 0
if (1 < 2 - 2)
  result = 1
end
verify(result, 0)
result = 0
if (2 + 3 < 6)
  result = 1
end
verify(result, 1)
)", 3);
}

TEST_F(parser, arguments_and_function_values) {
  run(R"(def subtract(first, second)
  return first - second
end
def apply(function, value)
  return function(value)
end
def increment(value)
  return value + 1
end
verify(subtract(10, 3), 7)
verify(apply(increment, 8), 9)
verify(subtract(increment(5), increment(2)), 3)
)", 3);
}

TEST_F(parser, recursive_calls_and_branch_boundaries) {
  run(R"(def fibonacci(n)
  if (n < 2)
    return n
  end
  return fibonacci(n - 2) + fibonacci(n - 1)
end
verify(fibonacci(0), 0)
verify(fibonacci(1), 1)
verify(fibonacci(2), 1)
verify(fibonacci(10), 55)
)", 4);
}

TEST_F(parser, empty_function_returns_none) {
  const char source[] = R"(def empty()
end
return empty()
)";
  ASSERT_EQ(candy_dostring(state, source, sizeof(source) - 1), CANDY_OK);
  EXPECT_EQ(candy_state_get_type(state, -1), CANDY_TYPE_NONE);
}

TEST_F(parser, literal_types) {
  const char floating[] = R"(return 1.25)";
  ASSERT_EQ(candy_dostring(state, floating, sizeof(floating) - 1), CANDY_OK);
  ASSERT_EQ(candy_state_get_type(state, -1), CANDY_TYPE_FLOAT);
  EXPECT_DOUBLE_EQ(candy_state_to_float(state, -1), 1.25);
  const char boolean[] = R"(return true)";
  ASSERT_EQ(candy_dostring(state, boolean, sizeof(boolean) - 1), CANDY_OK);
  EXPECT_EQ(candy_state_get_type(state, -1), CANDY_TYPE_BOOLEAN);
}

TEST_F(parser, syntax_errors_do_not_execute_or_poison_next_script) {
  const char *invalid[] = {
    R"(def incomplete(n)
return n
)",
    R"(verify(1 +, 1))",
    R"(if (1 < 2)
verify(1, 1)
)",
    R"(value = (1 + 2)"
  };
  for (const char *source : invalid) {
    SCOPED_TRACE(source);
    checks = 0;
    EXPECT_NE(candy_dostring(state, source, strlen(source)), CANDY_OK);
    EXPECT_EQ(checks, 0);
    run(R"(verify(2 + 3, 5))", 1);
  }
}
