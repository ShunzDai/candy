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
#include "src/candy_wrap.h"
#include "src/candy_gc.h"

struct object {
  candy_wrap_t obj;
  int data;
};

TEST(gc, root) {
  // candy_gc_t gc{};
  // candy_gc_init(&gc);
  // candy_gc_add(&gc, TYPE_NONE, sizeof(object));
  // candy_gc_add(&gc, TYPE_NONE, sizeof(object));
  // candy_gc_deinit(&gc);
}
