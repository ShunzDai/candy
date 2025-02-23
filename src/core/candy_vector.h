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
#ifndef CANDY_CORE_VECTOR_H
#define CANDY_CORE_VECTOR_H
#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#include "core/candy_memory.h"
#include "core/candy_priv.h"
#include <string.h>

struct candy_vector {
  void *data;
  size_t size;
  size_t cap;
};

static inline void candy_vector_set_capacity(candy_vector_t *self, size_t capacity) {
  self->cap = capacity;
}

static inline void candy_vector_set_size(candy_vector_t *self, size_t size) {
  self->size = size;
}

static inline void candy_vector_set_data(candy_vector_t *self, void *data) {
  self->data = data;
}

static inline size_t candy_vector_capacity(const candy_vector_t *self) {
  return self->cap;
}

static inline size_t candy_vector_size(const candy_vector_t *self) {
  return self->size;
}

static inline void *candy_vector_data(const candy_vector_t *self) {
  return self->data;
}

static inline candy_err_t candy_vector_init(candy_vector_t *self) {
  candy_vector_set_capacity(self, 0);
  candy_vector_set_size(self, 0);
  candy_vector_set_data(self, NULL);
  return CANDY_OK;
}

static inline candy_err_t candy_vector_deinit(candy_vector_t *self, candy_memory_t *mem, size_t cell) {
  candy_vector_set_data(self, candy_memory_free(mem, candy_vector_data(self),
    cell * candy_vector_capacity(self)
  ));
  candy_vector_set_size(self, 0);
  candy_vector_set_capacity(self, 0);
  return CANDY_OK;
}

static inline void candy_vector_reserve(candy_vector_t *self, candy_memory_t *mem, candy_excep_t *ctx, size_t capacity, size_t cell) {
  size_t cap = candy_vector_capacity(self);
  if (capacity <= cap)
    return;
  candy_vector_set_data(self, candy_memory_realloc(mem, ctx, candy_vector_data(self),
    cell * cap,
    cell * capacity
  ));
  candy_vector_set_capacity(self, capacity);
}

static inline candy_err_t candy_vector_append(candy_vector_t *self, candy_memory_t *mem, candy_excep_t *ctx, const void *data, size_t size, size_t cell) {
  size_t cap = candy_vector_capacity(self);
  size_t sz = candy_vector_size(self);
  if (cap < size + sz) {
    candy_vector_reserve(self, mem, ctx, cap + size, cell);
  }
  if (size && data) {
    memcpy((uint8_t *)candy_vector_data(self) + cell * sz, data, cell * size);
  }
  candy_vector_set_size(self, candy_vector_size(self) + size);
  return CANDY_OK;
}

static inline void candy_vector_resize(candy_vector_t *self, candy_memory_t *mem, candy_excep_t *ctx, size_t size, size_t cell) {
  size_t sz = candy_vector_size(self);
  if (sz < size)
    candy_vector_append(self, mem, ctx, NULL, size - sz, cell);
  else
    candy_vector_set_size(self, size);
}

#ifdef __cplusplus
}
#endif /* __cplusplus */
#endif /* CANDY_CORE_VECTOR_H */
