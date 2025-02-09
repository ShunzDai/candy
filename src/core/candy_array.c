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
#include "core/candy_array.h"
#include "core/candy_object.h"
#include "core/candy_gc.h"
#include "core/candy_vector.h"

struct candy_array {
  candy_object_t header;
  candy_vector_t vec;
  candy_object_t *gray;
};

candy_array_t *candy_array_create(candy_gc_t *gc, candy_excep_t *ctx, candy_types_t type, uint8_t mask) {
  candy_array_t *self = (candy_array_t *)candy_gc_add(gc, ctx, type, sizeof(candy_array_t));
  candy_object_set_mask((candy_object_t *)self, MASK_ARRAY | mask);
  self->gray = NULL;
  candy_vector_init(&self->vec, candy_type_size(type));
  return self;
}

candy_err_t candy_array_delete(candy_array_t *self, candy_gc_t *gc) {
  candy_vector_deinit(&self->vec, candy_gc_memory(gc));
  candy_gc_free(gc, self, sizeof(candy_array_t));
  return CANDY_OK;
}

candy_err_t candy_array_color(candy_array_t *self, candy_gc_t *gc) {
  switch (candy_object_get_type((candy_object_t *)self)) {
    case CANDY_TYPE_TABLE:
    case CANDY_TYPE_PROTO:
    case CANDY_TYPE_STATE:
      self->gray = candy_gc_gray_swap(gc, (candy_object_t *)self);
      candy_object_set_mark((candy_object_t *)self, MARK_GRAY);
      return CANDY_OK;
    default:
      candy_object_set_mark((candy_object_t *)self, MARK_DARK);
      return CANDY_OK;
  }
}

candy_err_t candy_array_diffuse(candy_array_t *self, candy_gc_t *gc) {
  candy_gc_gray_swap(gc, self->gray);
  candy_object_set_mark((candy_object_t *)self, MARK_DARK);
  /* traverse objects */
  candy_object_t *tail = (candy_object_t *)candy_array_data(self) + candy_array_size(self);
  for (candy_object_t *it = (candy_object_t *)candy_array_data(self); it < tail; ++it) {
    candy_gc_event_handler(gc)(it, gc, EVT_COLOR);
  }
  return CANDY_OK;
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

void candy_array_reserve(candy_array_t *self, candy_gc_t *gc, candy_excep_t *ctx, size_t capacity) {
  candy_vector_reserve(&self->vec, candy_gc_memory(gc), ctx, capacity);
}

void candy_array_resize(candy_array_t *self, candy_gc_t *gc, candy_excep_t *ctx, size_t size) {
  candy_vector_resize(&self->vec, candy_gc_memory(gc), ctx, size);
}

candy_err_t candy_array_append(candy_array_t *self, candy_gc_t *gc, candy_excep_t *ctx, const void *data, size_t size) {
  return candy_vector_append(&self->vec, candy_gc_memory(gc), ctx, data, size);
}
