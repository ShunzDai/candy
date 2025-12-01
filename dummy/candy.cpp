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
#include "candy.hpp"
#include <stdlib.h>

static void *_default_allocator(void *prev, size_t prev_size, size_t next_size, void *arg) {
  (void)prev_size, (void)arg;
  if (next_size)
    return realloc(prev, next_size);
  free(prev);
  return NULL;
}

candy::candy()
: self(candy_new_state(_default_allocator, this)) {

}
