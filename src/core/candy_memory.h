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
#ifndef CANDY_CORE_MEMORY_H
#define CANDY_CORE_MEMORY_H
#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#include "core/candy_priv.h"

struct candy_memory {
  size_t used;
  candy_allocator_t alloc;
  void *arg;
};

int candy_memory_init(candy_memory_t *self, candy_allocator_t alloc, void *arg);

int candy_memory_deinit(candy_memory_t *self);

void *candy_memory_realloc(candy_memory_t *self, candy_exce_t *ctx, void *prev, size_t prev_size, size_t next_size);

static inline void *candy_memory_alloc(candy_memory_t *self, candy_exce_t *ctx, size_t size) {
  return candy_memory_realloc(self, ctx, NULL, 0, size);
}

static inline void *candy_memory_free(candy_memory_t *self, void *ptr, size_t size) {
  return candy_memory_realloc(self, NULL, ptr, size, 0);
}

static inline size_t candy_memory_used(const candy_memory_t *self) {
  return self->used;
}

#ifdef __cplusplus
}
#endif /* __cplusplus */
#endif /* CANDY_CORE_MEMORY_H */
