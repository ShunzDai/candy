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

static void _set_capacity(candy_vector_t *self, size_t capacity) {
  self->cap = capacity;
}

static void _set_cell(candy_vector_t *self, size_t cell) {
  self->cell = cell;
}

static void _set_size(candy_vector_t *self, size_t size) {
  self->size = size;
}

static void _set_data(candy_vector_t *self, void *data) {
  self->data = data;
}

int candy_vector_init(candy_vector_t *self, size_t cell) {
  _set_capacity(self, 0);
  _set_cell(self, cell);
  _set_size(self, 0);
  _set_data(self, NULL);
  return 0;
}

void candy_vector_deinit(candy_vector_t *self, candy_gc_t *gc) {
  _set_data(self, candy_gc_alloc(gc, candy_vector_data(self),
    candy_vector_cell(self) * candy_vector_capacity(self),
    0
  ));
  _set_size(self, 0);
  _set_cell(self, 0);
  _set_capacity(self, 0);
}

void candy_vector_reserve(candy_vector_t *self, candy_gc_t *gc, size_t capacity) {
  size_t cap = candy_vector_capacity(self);
  if (capacity <= cap)
    return;
  _set_capacity(self, capacity);
  _set_data(self, candy_gc_alloc(gc, candy_vector_data(self),
    candy_vector_cell(self) * cap,
    candy_vector_cell(self) * candy_vector_capacity(self)
  ));
  memset(candy_vector_data(self), 0, candy_vector_cell(self) * candy_vector_capacity(self));
}

void candy_vector_resize(candy_vector_t *self, candy_gc_t *gc, size_t size) {
  size_t sz = candy_vector_size(self);
  if (size > sz)
    candy_vector_append(self, gc, NULL, size - sz);
  else
    _set_size(self, size);
}

int candy_vector_append(candy_vector_t *self, candy_gc_t *gc, const void *data, size_t size) {
  size_t cap = candy_vector_capacity(self);
  size_t sz = candy_vector_size(self);
  if (cap < size + sz) {
    _set_capacity(self, candy_vector_capacity(self) + size);
    _set_data(self, candy_gc_alloc(gc, candy_vector_data(self),
      candy_vector_cell(self) * cap,
      candy_vector_cell(self) * candy_vector_capacity(self)
    ));
  }
  if (size) {
    if (data)
      memcpy(candy_vector_data(self) + candy_vector_cell(self) * sz, data, candy_vector_cell(self) * size);
    else
      memset(candy_vector_data(self) + candy_vector_cell(self) * sz, 0, candy_vector_cell(self) * size);
  }
  _set_size(self, candy_vector_size(self) + size);
  return 0;
}
