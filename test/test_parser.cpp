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
#include <string>

class parser : public testing::Test {
protected:
  candy_state_t *state = nullptr;
  inline static int checks = 0;
  // Native callbacks own side-effect counters; script function assignments are local.
  inline static int probe_calls = 0;
  inline static int initializations = 0;
  inline static int steps = 0;
  inline static int loop_index = 0;
  inline static int inner_calls = 0;

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

  void reject_without_execution(const char *source) {
    SCOPED_TRACE(source);
    checks = 0;
    EXPECT_NE(candy_dostring(state, source, strlen(source)), CANDY_OK);
    EXPECT_EQ(checks, 0);
    run("verify(2 + 3, 5)", 1);
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

TEST_F(parser, direct_local_returns_preserve_values_and_recursive_self_identity) {
  run(R"(def first(a, b)
  return a
end
def second(a, b)
  return (b)
end
def identity(n)
  if (n < 1)
    return identity
  end
  return identity(n - 1)
end
verify(first(11, 22), 11)
verify(second(11, 22), 22)
verify(first(first(3, 4), second(5, 6)), 3)
f = identity(3)
g = f(0)
verify(first(7, 8) + second(9, 10), 17)
def count(n)
  if (n < 1)
    return n
  end
  return count(n - 1) + 1
end
verify(count(5), 5)
)", 5);
  ASSERT_EQ(candy_dostring(state, "return g(0)", 11), CANDY_OK);
  EXPECT_EQ(candy_state_get_type(state, -1), CANDY_TYPE_SCLOS);
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

TEST_F(parser, empty_function_returns_null) {
  const char source[] = R"(def empty()
end
return empty()
)";
  ASSERT_EQ(candy_dostring(state, source, sizeof(source) - 1), CANDY_OK);
  EXPECT_EQ(candy_state_get_type(state, -1), CANDY_TYPE_NULL);
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

TEST_F(parser, elif_else_selects_first_match_without_later_side_effects) {
  probe_calls = 0;
  ASSERT_EQ(candy_state_setglobal(state, "probe", +[](candy_state_t *current) -> int {
    ++probe_calls;
    EXPECT_EQ(candy_state_push_integer(current, candy_state_to_integer(current, 0)), CANDY_OK);
    return 1;
  }), CANDY_OK);
  ASSERT_EQ(candy_state_setglobal(state, "probe_count", +[](candy_state_t *current) -> int {
    EXPECT_EQ(candy_state_push_integer(current, probe_calls), CANDY_OK);
    return 1;
  }), CANDY_OK);
  run(R"(bodies = 0
if (0 < probe(0))
  bodies = 100
elif (0 < probe(1))
  bodies = bodies + 1
elif (0 < probe(1))
  bodies = 200
else
  bodies = 300
end
verify(probe_count(), 2)
verify(bodies, 1)
if (0 < probe(1))
  bodies = bodies + 10
elif (0 < probe(1))
  bodies = 400
else
  bodies = 500
end
verify(probe_count(), 3)
verify(bodies, 11)
if (0 < probe(0))
  bodies = 600
elif (0 < probe(0))
  bodies = 700
else
  bodies = bodies + 100
end
verify(probe_count(), 5)
verify(bodies, 111)
)", 6);
}

TEST_F(parser, nested_if_chains_keep_their_own_else_and_end) {
  run(R"(result = 0
if (true)
  if (false)
    result = 100
  elif (true)
    result = 1
  else
    result = 200
  end
elif (true)
  result = 300
else
  result = 400
end
if (false)
  result = 500
else
  if (false)
    result = 600
  else
    result = result + 10
  end
end
verify(result, 11)
)", 1);
}

TEST_F(parser, while_rechecks_condition_and_handles_nested_if_control) {
  run(R"(index = 0
sum = 0
while (index < 8)
  index = index + 1
  if (index < 3)
    continue
  elif (5 < index)
    break
  else
    sum = sum + index
  end
end
verify(index, 6)
verify(sum, 12)
while (false)
  sum = 999
  break
end
verify(sum, 12)
)", 3);
}

TEST_F(parser, nested_loops_break_and_continue_target_nearest_loop) {
  run(R"(outer = 0
visits = 0
sum = 0
while (outer < 3)
  outer = outer + 1
  inner = 0
  while (inner < 5)
    inner = inner + 1
    if (inner < 2)
      continue
    end
    if (3 < inner)
      break
    end
    visits = visits + 1
    sum = sum + outer
  end
  verify(inner, 4)
end
verify(outer, 3)
verify(visits, 6)
verify(sum, 12)
)", 6);
}

TEST_F(parser, for_assignment_clauses_and_zero_iterations) {
  run(R"(sum = 0
visits = 0
for (index : [0, 1, 2, 3, 4])
  visits = visits + 1
  if (8 < visits)
    break
  end
  sum = sum + index
end
verify(visits, 5)
verify(sum, 10)
for (index : [])
  sum = 999
  break
end
verify(sum, 10)
)", 3);
}

TEST_F(parser, for_continue_runs_step_and_break_skips_step) {
  run(R"(visits = 0
sum = 0
for (index : [0, 1, 2, 3, 4, 5])
  visits = visits + 1
  if (10 < visits)
    break
  end
  if (index < 2)
    continue
  elif (3 < index)
    break
  end
  sum = sum + index
end
verify(visits, 5)
verify(index, 4)
verify(sum, 5)
)", 3);
}

TEST_F(parser, for_empty_clauses_and_existing_name_or_call_clauses) {
  initializations = 0;
  steps = 0;
  loop_index = 2;
  ASSERT_EQ(candy_state_setglobal(state, "initialize", +[](candy_state_t *) -> int {
    ++initializations;
    loop_index = 0;
    return 0;
  }), CANDY_OK);
  ASSERT_EQ(candy_state_setglobal(state, "advance", +[](candy_state_t *) -> int {
    ++steps;
    ++loop_index;
    return 0;
  }), CANDY_OK);
  ASSERT_EQ(candy_state_setglobal(state, "initialization_count", +[](candy_state_t *current) -> int {
    EXPECT_EQ(candy_state_push_integer(current, initializations), CANDY_OK);
    return 1;
  }), CANDY_OK);
  ASSERT_EQ(candy_state_setglobal(state, "step_count", +[](candy_state_t *current) -> int {
    EXPECT_EQ(candy_state_push_integer(current, steps), CANDY_OK);
    return 1;
  }), CANDY_OK);
  ASSERT_EQ(candy_state_setglobal(state, "current_index", +[](candy_state_t *current) -> int {
    EXPECT_EQ(candy_state_push_integer(current, loop_index), CANDY_OK);
    return 1;
  }), CANDY_OK);
  run(R"(visits = 0
while (true)
  visits = visits + 1
  if (2 < visits)
    break
  end
end
verify(visits, 3)
index = 0
while (index < 2)
  index = index + 1
  if (4 < index)
    break
  end
end
verify(index, 2)
visits = 0
initialize()
while (current_index() < 3)
  visits = visits + 1
  if (5 < visits)
    break
  end
  advance()
end
verify(initialization_count(), 1)
verify(step_count(), 3)
verify(current_index(), 3)
verify(visits, 3)
visits = 0
while (visits < 2)
  visits = visits + 1
  if (4 < visits)
    break
  end
end
verify(visits, 2)
)", 7);
}

TEST_F(parser, nested_for_and_while_keep_continue_and_break_targets) {
  run(R"(visits = 0
total = 0
for (outer : [0, 1, 2])
  visits = visits + 1
  if (5 < visits)
    break
  end
  inner = 0
  while (inner < 4)
    inner = inner + 1
    if (inner < 2)
    elif (inner < 3)
      continue
    else
      break
    end
    total = total + 1
  end
  if (outer < 1)
  elif (outer < 2)
    continue
  end
  total = total + 10
end
verify(outer, 2)
verify(visits, 3)
verify(inner, 3)
verify(total, 23)
)", 4);
}

TEST_F(parser, nested_function_owns_its_loops_and_restores_outer_loop_scope) {
  inner_calls = 0;
  ASSERT_EQ(candy_state_setglobal(state, "record_inner_call", +[](candy_state_t *) -> int {
    ++inner_calls;
    return 0;
  }), CANDY_OK);
  ASSERT_EQ(candy_state_setglobal(state, "inner_call_count", +[](candy_state_t *current) -> int {
    EXPECT_EQ(candy_state_push_integer(current, inner_calls), CANDY_OK);
    return 1;
  }), CANDY_OK);
  run(R"(outer = 0
while (outer < 3)
  outer = outer + 1
  def inner()
    record_inner_call()
    local_index = 0
    while (local_index < 4)
      local_index = local_index + 1
      if (local_index < 2)
        continue
      end
      break
    end
    return local_index
  end
  verify(inner(), 2)
  if (outer < 3)
    continue
  end
  break
end
verify(outer, 3)
verify(inner_call_count(), 3)
)", 5);
}

TEST_F(parser, loop_returns_and_tail_calls_do_not_leak_function_scope) {
  run(R"(def identity(value)
  return value
end
def from_while(value)
  while (true)
    if (value < 0)
      return 7
    else
      return identity(value + 1)
    end
  end
end
def from_for(value)
  while (true)
    return from_while(value)
  end
end
def nested(value)
  while (true)
    def inner(number)
      while (true)
        return identity(number + 2)
      end
    end
    return inner(value)
  end
end
verify(from_while(4), 5)
verify(from_while(0 - 1), 7)
verify(from_for(8), 9)
verify(nested(10), 12)
index = 0
while (index < 2)
  index = index + 1
  verify(from_for(index), index + 1)
end
verify(index, 2)
)", 7);
}

TEST_F(parser, malformed_control_flow_is_rejected_before_any_execution) {
  const char *invalid[] = {
    "verify(1, 1)\nif (true)\nverify(2, 2)\n",
    "verify(1, 1)\nif (false)\nelif true\nend\n",
    "verify(1, 1)\nif (false)\nelif (true\nend\n",
    "verify(1, 1)\nif (true)\nelse (false)\nend\n",
    "verify(1, 1)\nif (true)\nelse\nelif (true)\nend\n",
    "verify(1, 1)\nif (true)\nelse\nelse\nend\n",
    "verify(1, 1)\nelif (true)\nend\n",
    "verify(1, 1)\nelse\nend\n",
    "verify(1, 1)\nwhile true\nend\n",
    "verify(1, 1)\nwhile (true\nend\n",
    "verify(1, 1)\nwhile (false)\n",
    "verify(1, 1)\nfor (index : [0, 1)\nend\n",
    "verify(1, 1)\nfor (index : 0 1)\nend\n",
    "verify(1, 1)\nfor (index : [0, 1]\nend\n",
    "verify(1, 1)\nwhile (true)\n",
    "verify(1, 1)\nfor (1 + 2 : [0])\nend\n",
    "verify(1, 1)\nfor (: [0])\nend\n",
    "verify(1, 1)\nfor (index : 1 + 2\nend\n",
    "verify(1, 1)\nbreak\n",
    "verify(1, 1)\ncontinue\n",
    "verify(1, 1)\nif (true)\nbreak\nend\n",
    "verify(1, 1)\nwhile (false)\nend\nbreak\n",
    "verify(1, 1)\nfor (x : [0])\nend\ncontinue\n",
    "verify(1, 1)\nimport unsupported\n",
  };
  for (const char *source : invalid) {
    reject_without_execution(source);
  }
}

TEST_F(parser, nested_function_cannot_break_or_continue_enclosing_loop) {
  const char *invalid[] = {
    "verify(1, 1)\nwhile (false)\ndef inner()\nbreak\nend\nend\n",
    "verify(1, 1)\nwhile (false)\ndef inner()\ncontinue\nend\nend\n",
    "verify(1, 1)\nfor (x : [0])\ndef inner()\nbreak\nend\nend\n",
    "verify(1, 1)\nfor (x : [0])\ndef inner()\ncontinue\nend\nend\n",
    "verify(1, 1)\nwhile (false)\ndef inner()\nwhile (false)\nend\nbreak\nend\nend\n",
    "verify(1, 1)\ndef outer()\nwhile (false)\nend\ncontinue\nend\n",
  };
  for (const char *source : invalid) {
    reject_without_execution(source);
  }
}

TEST_F(parser, many_forward_exits_and_nested_branches_patch_after_growth) {
  std::string source = "visits = 0\nwhile (true)\n";
  for (int i = 0; i < 1000; ++i)
    source += "if (false)\nbreak\nelif (false)\nbreak\nend\n";
  source += "visits = visits + 1\nbreak\nend\nverify(visits, 1)\n";
  run(source.c_str(), 1);
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

TEST_F(parser, multiplication_division_and_modulo) {
  run(R"(verify(3 * 4, 12)
verify(7 / 2, 3)
verify(10 % 3, 1)
verify(2 + 3 * 4, 14)
verify((2 + 3) * 4, 20)
verify(1 - 6 / 3, -1)
)", 6);
}

TEST_F(parser, table_literal_and_subscript) {
  run(R"(a = {10, 20, 30}
verify(a[0], 10)
verify(a[1] + a[2], 50)
a[0] = 99
verify(a[0], 99)
verify({5, 6, 7}[1], 6)
)", 4);
}

TEST_F(parser, float_arithmetic) {
  const char source[] = R"(return 1.5 + 2.5)";
  ASSERT_EQ(candy_dostring(state, source, sizeof(source) - 1), CANDY_OK);
  ASSERT_EQ(candy_state_get_type(state, -1), CANDY_TYPE_FLOAT);
  EXPECT_DOUBLE_EQ(candy_state_to_float(state, -1), 4.0);
}

TEST_F(parser, unary_minus) {
  run(R"(verify(-1, 0 - 1)
verify(-2 + 3, 1)
verify(-2 * 3, 0 - 6)
)", 3);
}

TEST_F(parser, json_object_literal) {
  run(R"(a = {"x": 1, "y": 2}
verify(a["x"], 1)
verify(a["y"], 2)
verify(a["x"] + a["y"], 3)
)", 3);
}

TEST_F(parser, json_array_literal) {
  run(R"(a = [1, 2, 3]
verify(a[0], 1)
verify(a[1] + a[2], 5)
b = [{"x": 4}, {"x": 5}]
verify(b[0]["x"], 4)
verify(b[1]["x"], 5)
)", 4);
}

TEST_F(parser, large_json_literals_reuse_temporary_registers) {
  std::string source = "a = [";
  for (int i = 0; i < 300; ++i) {
    if (i)
      source += ",";
    source += std::to_string(i);
  }
  source += "]\nverify(a[299], 299)\no = {";
  for (int i = 0; i < 300; ++i) {
    if (i)
      source += ",";
    source += "\"k" + std::to_string(i) + "\":" + std::to_string(i);
  }
  source += "}\nverify(o[\"k299\"], 299)\n";
  run(source.c_str(), 2);
}

TEST_F(parser, equality_and_relational_operators) {
  run(R"(count = 0
if (1 == 1)
  count = count + 1
end
if (1 == 2)
  count = count + 1
end
if (1 != 2)
  count = count + 1
end
if (5 > 3)
  count = count + 1
end
if (5 >= 5)
  count = count + 1
end
if (3 <= 5)
  count = count + 1
end
verify(count, 5)
)", 1);
}

TEST_F(parser, bitwise_operators) {
  run(R"(verify(6 & 3, 2)
verify(6 | 3, 7)
verify(6 ^ 3, 5)
verify(1 << 4, 16)
verify(16 >> 2, 4)
verify(1 << 4 & 7, 0)
)", 6);
}

TEST_F(parser, logical_and_or_not_short_circuit) {
  probe_calls = 0;
  ASSERT_EQ(candy_state_setglobal(state, "probe", +[](candy_state_t *current) -> int {
    ++probe_calls;
    EXPECT_EQ(candy_state_push_integer(current, 1), CANDY_OK);
    return 1;
  }), CANDY_OK);
  ASSERT_EQ(candy_state_setglobal(state, "probe_count", +[](candy_state_t *current) -> int {
    EXPECT_EQ(candy_state_push_integer(current, probe_calls), CANDY_OK);
    return 1;
  }), CANDY_OK);
  run(R"(count = 0
if (true && true)
  count = count + 1
end
if (true && false)
  count = count + 1
end
if (false || true)
  count = count + 1
end
if (!false)
  count = count + 1
end
if (!null)
  count = count + 1
end
verify(count, 4)
x = false && probe()
verify(probe_count(), 0)
y = true || probe()
verify(probe_count(), 0)
z = false || probe()
verify(probe_count(), 1)
)", 4);
}

TEST_F(parser, compound_assignment) {
  run(R"(a = 10
a += 5
a -= 3
a *= 2
a /= 4
verify(a, 6)
b = 7
b %= 3
c = 6
c &= 3
c |= 8
c ^= 1
verify(b, 1)
verify(c, 11)
)", 3);
}

TEST_F(parser, for_range_iteration) {
  run(R"(sum = 0
for (x : [1, 2, 3, 4])
  sum = sum + x
end
verify(sum, 10)
)", 1);
}

TEST_F(parser, addition_rhs_uses_shared_precedence_parser) {
  run(R"(a = 2
verify(1 + a * 3, 7)
verify(1 + a * 3 + 4, 11)
values = [2, 3]
verify(1 + values[0] * values[1], 7)
def calculate(a, b)
  return 1 + a * b
end
verify(calculate(2, 3), 7)
)", 4);
}

TEST_F(parser, literal_array_prefix_continues_expression) {
  run(R"(a = [1 + 2, 4 * 5, 6]
verify(a[0], 3)
verify(a[1], 20)
verify(a[2], 6)
b = [1, 2, 3 + 4, a[0]]
verify(b[2], 7)
verify(b[3], 3)
)", 5);
}

TEST_F(parser, logical_assignment_preserves_rhs_local_reads) {
  run(R"(def check()
  x = 5
  x = x && x == 5
  count = 0
  if (x)
    count += 1
  end
  y = null
  y = y || y == null
  if (y)
    count += 1
  end
  return count
end
verify(check(), 2)
)", 1);
}

TEST_F(parser, range_for_reuses_existing_function_local) {
  run(R"(def count()
  x = 10
  sum = 0
  for (x : [1, 2])
    sum += x
  end
  for (x : [3, 4])
    sum += x
  end
  return sum + x
end
verify(count(), 14)
)", 1);
}

TEST_F(parser, global_in_place_add_is_fused_and_correct) {
  run(R"(x = 1
y = 2
x = x + y
verify(x, 3)
x = x + 10
verify(x, 13)
x = x + y * 2
verify(x, 17)
x = x + y + 1
verify(x, 20)
f = 1.5
f = f + 0.25
g = 0
i = 0
while (i < 5)
  g = g + i
  i = i + 1
end
verify(g, 10)
)", 5);
  const char source[] = R"(f = 1.5
f = f + 0.25
return f
)";
  ASSERT_EQ(candy_dostring(state, source, sizeof(source) - 1), CANDY_OK);
  ASSERT_EQ(candy_state_get_type(state, -1), CANDY_TYPE_FLOAT);
  EXPECT_DOUBLE_EQ(candy_state_to_float(state, -1), 1.75);
}

TEST_F(parser, block_local_survives_loop_condition_registers) {
  run(R"(def count_down(n)
  while (n > 0)
    last = n
    n = n - 1
  end
  return last
end
verify(count_down(3), 1)
verify(count_down(7), 1)
def sum_to(n)
  total = 0
  while (n > 0)
    step = n * 2
    total = total + step
    n = n - 1
  end
  return total + step
end
verify(sum_to(3), 14)
)", 3);
}
