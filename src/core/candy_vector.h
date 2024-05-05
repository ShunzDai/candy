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
#ifndef CANDY_CORE_VECTOR_H
#define CANDY_CORE_VECTOR_H
#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#include "core/candy_priv.h"

struct candy_vector {
  size_t cap;
  size_t cell;
  size_t size;
  void *data;
};

int candy_vector_init(candy_vector_t *self, size_t cell);
void candy_vector_deinit(candy_vector_t *self, candy_gc_t *gc);

void candy_vector_reserve(candy_vector_t *self, candy_gc_t *gc, size_t capacity);

void candy_vector_resize(candy_vector_t *self, candy_gc_t *gc, size_t size);

int candy_vector_append(candy_vector_t *self, candy_gc_t *gc, const void *data, size_t size);

static inline size_t candy_vector_capacity(const candy_vector_t *self) {
  return self->cap;
}

static inline size_t candy_vector_size(const candy_vector_t *self) {
  return self->size;
}

static inline void *candy_vector_data(const candy_vector_t *self) {
  return self->data;
}

#ifdef __cplusplus
}
#endif /* __cplusplus */
#endif /* CANDY_CORE_VECTOR_H */
