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
#include "core/candy_array.h"
#include "core/candy_object.h"
#include "core/candy_gc.h"
#include "core/candy_vector.h"

struct candy_array {
  candy_object_t header;
  candy_vector_t vec;
};

candy_array_t *candy_array_create(candy_gc_t *gc, candy_types_t type, size_t cell) {
  candy_array_t *self = (candy_array_t *)candy_gc_add(gc, type, sizeof(struct candy_array));
  candy_vector_init(&self->vec, cell);
  return self;
}

int candy_array_delete(candy_array_t *self, candy_gc_t *gc) {
  candy_vector_deinit(&self->vec, gc);
  candy_gc_alloc(gc, self, sizeof(struct candy_array), 0);
  return 0;
}

size_t candy_array_capacity(const candy_array_t *self) {
  return candy_vector_capacity(&self->vec);
}

size_t candy_array_size(const candy_array_t *self) {
  return candy_vector_size(&self->vec);
}

void *candy_array_data(const candy_array_t *self) {
  return candy_vector_data(&self->vec);
}

candy_vector_t *candy_array_vector(candy_array_t *self) {
  return &self->vec;
}

void candy_array_resize(candy_array_t *self, candy_gc_t *gc, size_t size) {
  candy_vector_resize(&self->vec, gc, size);
}

int candy_array_append(candy_array_t *self, candy_gc_t *gc, const void *data, size_t size) {
  return candy_vector_append(&self->vec, gc, data, size);
}
