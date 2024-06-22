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
#include "core/candy_memory.h"
#include "core/candy_exception.h"
#include <stdlib.h>

int candy_memory_init(candy_memory_t *self, candy_allocator_t alloc, void *arg) {
  self->used = 0;
  self->alloc = alloc;
  self->arg = arg;
  return 0;
}

int candy_memory_deinit(candy_memory_t *self) {
  self->arg = NULL;
  self->alloc = NULL;
  assert(self->used == 0);
  return 0;
}

void *candy_memory_realloc(candy_memory_t *self, candy_exce_t *ctx, void *prev, size_t prev_size, size_t next_size) {
  assert((prev_size == 0) == (prev == NULL));
  void *next = self->alloc(prev, prev_size, next_size, self->arg);
  if (next_size && next == NULL) {
    if (ctx)
      candy_exce_throw(ctx, EXCE_ERR_MEMORY, NULL);
    else
      abort();
  }
  self->used += next_size - prev_size;
  return next;
}
