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

static candy_err_t _event_handler(candy_object_t *self, candy_gc_t *gc, candy_events_t evt, void *arg) {
  if (candy_object_mask(self) & MASK_ARRAY)
    return candy_array_handler((candy_array_t *)self, gc, evt, arg);
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
  candy_gc_init(&gc, _event_handler, test_allocator, nullptr);
  candy_table_t *self = candy_table_create(&gc, nullptr);
  candy_integer_t k[num], v[num];
  for (size_t idx = 0; idx < num; ++idx) {
    k[idx] = rand();
    v[idx] = rand();
    candy_wrap_t key{}, val{};
    candy_wrap_set_integer(&key, k[idx]);
    candy_wrap_set_integer(&val, v[idx]);
    candy_table_set(self, &gc, nullptr, &key, &val);
  }
  candy_table_fprint(self, stdout);
  for (size_t idx = 0; idx < num; ++idx) {
    candy_wrap_t key{};
    candy_wrap_set_integer(&key, k[idx]);
    EXPECT_EQ(candy_wrap_get_integer(candy_table_get(self, &gc, &key)), v[idx]);
  }
  candy_gc_deinit(&gc);
}

TEST(table, reset) {
  constexpr int num = 10;
  candy_gc_t gc{};
  candy_gc_init(&gc, (candy_handler_t)candy_table_handler, test_allocator, nullptr);
  candy_table_t *self = candy_table_create(&gc, nullptr);
  candy_integer_t k[num], v[num];
  for (size_t idx = 0; idx < num; ++idx) {
    k[idx] = rand();
    v[idx] = rand();
    candy_wrap_t key{}, val{};
    candy_wrap_set_integer(&key, k[idx]);
    candy_wrap_set_integer(&val, v[idx]);
    candy_table_set(self, &gc, nullptr, &key, &val);
    if (k[idx] % 3) {
      candy_table_reset(self, &gc, &key);
      k[idx] = 0;
    }
  }
  candy_table_fprint(self, stdout);
  for (size_t idx = 0; idx < num; ++idx) {
    candy_wrap_t key{};
    if (k[idx] == 0)
      continue;
    candy_wrap_set_integer(&key, k[idx]);
    EXPECT_EQ(candy_wrap_get_integer(candy_table_get(self, &gc, &key)), v[idx]);
  }
  candy_gc_deinit(&gc);
}

TEST(table, key_obj) {
  constexpr int num = 10;
  candy_gc_t gc{};
  candy_gc_init(&gc, (candy_handler_t)_event_handler, test_allocator, nullptr);
  candy_table_t *self = candy_table_create(&gc, nullptr);
  candy_object_t *k[num];
  candy_integer_t v[num];
  for (size_t idx = 0; idx < num; ++idx) {
    auto s = std::to_string(rand());
    k[idx] = (candy_object_t *)candy_array_create_static(&gc, nullptr, CANDY_TYPE_CHAR, s.data(), s.size());
    v[idx] = rand();
    candy_wrap_t key{}, val{};
    candy_wrap_set_object(&key, k[idx]);
    candy_wrap_set_integer(&val, v[idx]);
    candy_table_set(self, &gc, nullptr, &key, &val);
  }
  candy_table_fprint(self, stdout);
  for (size_t idx = 0; idx < num; ++idx) {
    candy_wrap_t key{};
    candy_wrap_set_object(&key, k[idx]);
    EXPECT_EQ(candy_wrap_get_integer(candy_table_get(self, &gc, &key)), v[idx]);
  }
  candy_gc_deinit(&gc);
}
