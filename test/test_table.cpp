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

static candy_err_t _event_handler(candy_object_t *self, candy_gc_t *gc, candy_events_t evt, void *arg) {
  if (candy_object_mask(self) & MASK_ARRAY) {
    return candy_array_handler((candy_array_t *)self, gc, evt, arg);
  }
  switch (candy_object_type(self)) {
    case CANDY_TYPE_CCLOS: return candy_cclosure_handler((candy_cclosure_t *)self, gc, evt, arg);
    case CANDY_TYPE_SCLOS: return candy_sclosure_handler((candy_sclosure_t *)self, gc, evt, arg);
    case CANDY_TYPE_TABLE: return candy_table_handler((candy_table_t *)self, gc, evt, arg);
    case CANDY_TYPE_PROTO: return candy_proto_handler((candy_proto_t *)self, gc, evt, arg);
    case CANDY_TYPE_STATE: return candy_state_handler((candy_state_t *)self, gc, evt, arg);
    default:               return CANDY_ERR;
  }
}

TEST(table, fill) {
  constexpr int num = 10;
  candy_gc_t gc{};
  candy_gc_init(&gc, nullptr, _event_handler, test_allocator, nullptr);
  candy_table_t *self = candy_table_create(&gc, nullptr);
  candy_integer_t k[num], v[num];
  for (size_t idx = 0; idx < num; ++idx) {
    k[idx] = static_cast<candy_integer_t>(idx * 256 + 1);
    v[idx] = static_cast<candy_integer_t>(idx * 17 + 100);
    candy_wrap_t key{}, val{};
    candy_wrap_set_integer(&key, k[idx]);
    candy_wrap_set_integer(&val, v[idx]);
    ASSERT_EQ(candy_table_set(self, &gc, nullptr, &key, &val), CANDY_OK);
  }
  for (size_t idx = 0; idx < num; ++idx) {
    candy_wrap_t key{};
    candy_wrap_set_integer(&key, k[idx]);
    const candy_wrap_t *value = candy_table_get(self, &gc, &key);
    ASSERT_EQ(candy_wrap_type(value), CANDY_TYPE_INTEGER);
    EXPECT_EQ(candy_wrap_get_integer(value), v[idx]);
  }
  candy_gc_deinit(&gc);
}

TEST(table, reset) {
  constexpr int num = 10;
  candy_gc_t gc{};
  candy_gc_init(&gc, nullptr, _event_handler, test_allocator, nullptr);
  candy_table_t *self = candy_table_create(&gc, nullptr);
  candy_integer_t k[num], v[num];
  for (size_t idx = 0; idx < num; ++idx) {
    k[idx] = static_cast<candy_integer_t>(idx * 256 + 1);
    v[idx] = static_cast<candy_integer_t>(idx + 100);
    candy_wrap_t key{}, val{};
    candy_wrap_set_integer(&key, k[idx]);
    candy_wrap_set_integer(&val, v[idx]);
    ASSERT_EQ(candy_table_set(self, &gc, nullptr, &key, &val), CANDY_OK);
  }
  for (size_t idx = 0; idx < num; idx += 2) {
    candy_wrap_t key{};
    candy_wrap_set_integer(&key, k[idx]);
    ASSERT_EQ(candy_table_reset(self, &gc, &key), CANDY_OK);
  }
  for (size_t idx = 0; idx < num; ++idx) {
    candy_wrap_t key{};
    candy_wrap_set_integer(&key, k[idx]);
    const candy_wrap_t *value = candy_table_get(self, &gc, &key);
    if (idx % 2 == 0) {
      EXPECT_EQ(candy_wrap_type(value), CANDY_TYPE_NULL);
    } else {
      ASSERT_EQ(candy_wrap_type(value), CANDY_TYPE_INTEGER);
      EXPECT_EQ(candy_wrap_get_integer(value), v[idx]);
    }
  }
  for (size_t idx = 0; idx < num; idx += 2) {
    candy_wrap_t key{}, value{};
    candy_wrap_set_integer(&key, k[idx]);
    candy_wrap_set_integer(&value, -v[idx]);
    ASSERT_EQ(candy_table_set(self, &gc, nullptr, &key, &value), CANDY_OK);
  }
  for (size_t idx = 0; idx < num; ++idx) {
    candy_wrap_t key{};
    candy_wrap_set_integer(&key, k[idx]);
    const candy_wrap_t *value = candy_table_get(self, &gc, &key);
    ASSERT_EQ(candy_wrap_type(value), CANDY_TYPE_INTEGER);
    EXPECT_EQ(candy_wrap_get_integer(value), idx % 2 == 0 ? -v[idx] : v[idx]);
  }
  candy_gc_deinit(&gc);
}

TEST(table, key_obj) {
  constexpr int num = 10;
  candy_gc_t gc{};
  candy_gc_init(&gc, nullptr, _event_handler, test_allocator, nullptr);
  candy_table_t *self = candy_table_create(&gc, nullptr);
  candy_object_t *k[num];
  candy_integer_t v[num];
  for (size_t idx = 0; idx < num; ++idx) {
    auto s = std::string("key_") + std::to_string(idx);
    k[idx] = (candy_object_t *)candy_array_create_const(&gc, nullptr, CANDY_TYPE_CHAR, s.data(), s.size());
    v[idx] = static_cast<candy_integer_t>(idx + 200);
    candy_wrap_t key{}, val{};
    candy_wrap_set_object(&key, k[idx]);
    candy_wrap_set_integer(&val, v[idx]);
    ASSERT_EQ(candy_table_set(self, &gc, nullptr, &key, &val), CANDY_OK);
  }
  for (size_t idx = 0; idx < num; ++idx) {
    candy_wrap_t key{};
    candy_wrap_set_object(&key, k[idx]);
    const candy_wrap_t *value = candy_table_get(self, &gc, &key);
    ASSERT_EQ(candy_wrap_type(value), CANDY_TYPE_INTEGER);
    EXPECT_EQ(candy_wrap_get_integer(value), v[idx]);
  }
  candy_gc_deinit(&gc);
}

TEST(table, overwrite_and_missing_keys) {
  candy_gc_t gc{};
  ASSERT_EQ(candy_gc_init(&gc, nullptr, _event_handler, test_allocator, nullptr), CANDY_OK);
  candy_table_t *self = candy_table_create(&gc, nullptr);
  candy_wrap_t key{}, value{};
  candy_wrap_set_integer(&key, 42);
  EXPECT_EQ(candy_wrap_type(candy_table_get(self, &gc, &key)), CANDY_TYPE_NULL);
  EXPECT_EQ(candy_table_reset(self, &gc, &key), CANDY_OK);
  for (candy_integer_t expected : {10, 20, -30}) {
    candy_wrap_set_integer(&value, expected);
    ASSERT_EQ(candy_table_set(self, &gc, nullptr, &key, &value), CANDY_OK);
    const candy_wrap_t *actual = candy_table_get(self, &gc, &key);
    ASSERT_EQ(candy_wrap_type(actual), CANDY_TYPE_INTEGER);
    EXPECT_EQ(candy_wrap_get_integer(actual), expected);
  }
  EXPECT_EQ(candy_table_reset(self, &gc, &key), CANDY_OK);
  EXPECT_EQ(candy_table_reset(self, &gc, &key), CANDY_OK);
  EXPECT_EQ(candy_wrap_type(candy_table_get(self, &gc, &key)), CANDY_TYPE_NULL);
  candy_gc_deinit(&gc);
}

TEST(table, overwrite_beyond_tombstone_does_not_duplicate_key) {
  candy_gc_t gc{};
  ASSERT_EQ(candy_gc_init(&gc, nullptr, _event_handler, test_allocator, nullptr), CANDY_OK);
  auto *self = candy_table_create(&gc, nullptr);
  candy_wrap_t first{}, second{}, value{};
  candy_wrap_set_integer(&first, 1);
  candy_wrap_set_integer(&second, 257);
  candy_wrap_set_integer(&value, 10);
  ASSERT_EQ(candy_table_set(self, &gc, nullptr, &first, &value), CANDY_OK);
  ASSERT_EQ(candy_table_set(self, &gc, nullptr, &second, &value), CANDY_OK);
  ASSERT_EQ(candy_table_reset(self, &gc, &first), CANDY_OK);
  candy_wrap_set_integer(&value, 20);
  ASSERT_EQ(candy_table_set(self, &gc, nullptr, &second, &value), CANDY_OK);
  const candy_wrap_t *actual = candy_table_get(self, &gc, &second);
  ASSERT_EQ(candy_wrap_type(actual), CANDY_TYPE_INTEGER);
  EXPECT_EQ(candy_wrap_get_integer(actual), 20);
  ASSERT_EQ(candy_table_reset(self, &gc, &second), CANDY_OK);
  EXPECT_EQ(candy_wrap_type(candy_table_get(self, &gc, &second)), CANDY_TYPE_NULL);
  candy_gc_deinit(&gc);
}
