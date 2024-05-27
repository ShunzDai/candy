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
#include "core/candy_array.h"
#include "core/candy_table.h"
#include "core/candy_gc.h"
#include "core/candy_wrap.h"

static int handler(candy_object_t *self, candy_gc_t *gc, candy_events_t evt) {
  return candy_table_delete((candy_table_t *)self, gc);
}

TEST(table, init) {
  candy_gc_t gc{};
  candy_gc_init(&gc, handler, test_allocator, nullptr);
  candy_table_t *self = candy_table_create(&gc);
  for (size_t count = 0; count < 0xF; ++count) {
    candy_wrap_t key{}, val{};
    candy_wrap_set_integer(&key, rand());
    candy_wrap_set_integer(&val, rand());
    candy_table_set(self, &gc, &key, &val);
    EXPECT_EQ(candy_wrap_get_integer(candy_table_get(self, &key)), candy_wrap_get_integer(&val));
  }
  // candy_table_fprint(self, stdout);
  candy_gc_deinit(&gc);
}
