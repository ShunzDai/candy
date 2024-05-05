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
#include "core/candy_vector.h"
#include "core/candy_object.h"
#include "core/candy_gc.h"
#include "core/candy_lib.h"
#include <string.h>

int candy_vector_init(candy_vector_t *self, size_t cell) {
  self->cap = 0;
  self->cell = cell;
  self->size = 0;
  self->data = NULL;
  return 0;
}

void candy_vector_deinit(candy_vector_t *self, candy_gc_t *gc) {
  candy_gc_alloc(gc, self->data, candy_vector_capacity(self), 0);
  self->data = NULL;
  self->size = 0;
  self->cell = 0;
  self->cap = 0;
}

void candy_vector_reserve(candy_vector_t *self, candy_gc_t *gc, size_t capacity) {
  size_t cap = candy_vector_capacity(self);
  if (capacity <= cap)
    return;
  self->cap = capacity;
  self->data = candy_gc_alloc(gc, self->data, self->cell * cap, self->cell * self->cap);
  memset(self->data, 0, self->cell * self->cap);
}

void candy_vector_resize(candy_vector_t *self, candy_gc_t *gc, size_t size) {
  size_t sz = candy_vector_size(self);
  if (size > sz)
    candy_vector_append(self, gc, NULL, size - sz);
  else
    self->size = size;
}

int candy_vector_append(candy_vector_t *self, candy_gc_t *gc, const void *data, size_t size) {
  size_t cap = candy_vector_capacity(self);
  size_t sz = candy_vector_size(self);
  if (cap < size + sz) {
    self->cap += size;
    self->data = candy_gc_alloc(gc, self->data, self->cell * cap, self->cell * self->cap);
  }
  if (size) {
    if (data)
      memcpy(self->data + self->cell * sz, data, self->cell * size);
    else
      memset(self->data + self->cell * sz, 0, self->cell * size);
  }
  self->size += size;
  return 0;
}
