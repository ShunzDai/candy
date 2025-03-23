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
#include "core/candy_lib.h"
#include "core/candy_object.h"
#include "core/candy_exception.h"
#include "core/candy_gc.h"
#include "core/candy_vector.h"
#include <string.h>

typedef struct candy_dynamic candy_dynamic_t;
typedef struct candy_static candy_static_t;

struct candy_array {
  candy_object_t header;
  candy_hash_t hash;
  candy_object_t *gray;
};

struct candy_dynamic {
  candy_array_t arr;
  candy_vector_t vec;
};

struct candy_static {
  candy_array_t arr;
  size_t size;
  uint8_t data[];
};

#define arr_assert(_condition, _format, ...) \
candy_assert(ctx, gc, _condition, CANDY_ERR_ARRAY, _format, ##__VA_ARGS__)

static inline bool _is_const(const candy_array_t *self) {
  return candy_object_mask((candy_object_t *)self) & MASK_CONST;
}

static candy_err_t _array_delete(candy_array_t *self, candy_gc_t *gc, void *arg) {
  size_t cell = candy_type_size(candy_object_type((candy_object_t *)self));
  if (_is_const(self)) {
    candy_gc_free(gc, self, sizeof(candy_static_t) + cell * (candy_array_size(self) + 1));
  }
  else {
    candy_vector_deinit(&((candy_dynamic_t *)self)->vec, candy_gc_memory(gc), cell);
    candy_gc_free(gc, self, sizeof(candy_dynamic_t));
  }
  return CANDY_OK;
}

static candy_err_t _array_color(candy_array_t *self, candy_gc_t *gc, void *arg) {
  switch (candy_object_type((candy_object_t *)self)) {
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

static candy_err_t _array_diffuse(candy_array_t *self, candy_gc_t *gc, void *arg) {
  candy_gc_gray_swap(gc, self->gray);
  candy_object_set_mark((candy_object_t *)self, MARK_DARK);
  /* traverse objects */
  candy_object_t *tail = (candy_object_t *)candy_array_data(self) + candy_array_size(self);
  for (candy_object_t *it = (candy_object_t *)candy_array_data(self); it < tail; ++it) {
    candy_gc_event_handler(gc)(it, gc, EVT_COLOR, arg);
  }
  return CANDY_OK;
}

static candy_err_t _array_hash(candy_array_t *self, candy_gc_t *gc, void *arg) {
  *(candy_hash_t *)arg = self->hash;
  return CANDY_OK;
}

candy_array_t *candy_array_create(candy_gc_t *gc, candy_excep_t *ctx, candy_types_t type) {
  candy_dynamic_t *self = (candy_dynamic_t *)candy_gc_add(gc, ctx, type, sizeof(candy_dynamic_t));
  candy_object_set_mask((candy_object_t *)self, MASK_ARRAY);
  self->arr.hash = 0;
  self->arr.gray = NULL;
  candy_vector_init(&((candy_dynamic_t *)self)->vec);
  return (candy_array_t *)self;
}

candy_array_t *candy_array_create_const(candy_gc_t *gc, candy_excep_t *ctx, candy_types_t type, const void *data, size_t size) {
  size_t len = candy_type_size(type) * size;
  candy_hash_t hash = hash_djb(data, len);
  candy_static_t *self = (candy_static_t *)candy_gc_bloom_filter(gc, hash);
  if (
    self == NULL ||
    candy_object_type((candy_object_t *)self) != type ||
    self->size != size ||
    memcmp(self->data, data, size) != 0
  ) {
    self = (candy_static_t *)candy_gc_add_pool(gc, ctx, type,
      sizeof(candy_static_t) + len + candy_type_size(type),
      hash
    );
    candy_object_set_mask((candy_object_t *)self, MASK_ARRAY | MASK_CONST);
    memcpy(self->data, data, len);
    memset(self->data + len, 0, candy_type_size(type));
    self->arr.hash = hash;
    self->arr.gray = NULL;
    self->size = size;
  }
  return (candy_array_t *)self;
}

candy_array_t *candy_array_vprint(candy_gc_t *gc, candy_excep_t *ctx, const char format[], va_list args) {
  va_list args_copy;
  va_copy(args_copy, args);
  int len = vsnprintf(NULL, 0, format, args_copy);
  candy_static_t *self = (candy_static_t *)candy_gc_add(gc, ctx, CANDY_TYPE_CHAR,
    sizeof(candy_static_t) + len + candy_type_size(CANDY_TYPE_CHAR)
  );
  candy_object_set_mask((candy_object_t *)self, MASK_ARRAY | MASK_CONST);
  vsnprintf((char *)self->data, len + 1, format, args);
  self->arr.hash = hash_djb(self->data, len);
  self->arr.gray = NULL;
  self->size = len;
  return (candy_array_t *)self;
}

candy_array_t *candy_array_print(candy_gc_t *gc, candy_excep_t *ctx, const char format[], ...) {
  va_list args;
	va_start(args, format);
	candy_array_t *out = candy_array_vprint(gc, ctx, format, args);
	va_end(args);
  return out;
}

candy_err_t candy_array_handler(candy_array_t *self, candy_gc_t *gc, candy_events_t evt, void *arg) {
  switch (evt) {
    case EVT_DELETE:  return _array_delete(self, gc, arg);
    case EVT_COLOR:   return _array_color(self, gc, arg);
    case EVT_DIFFUSE: return _array_diffuse(self, gc, arg);
    case EVT_HASH:    return _array_hash(self, gc, arg);
    default:          return CANDY_ERR;
  }
}

size_t candy_array_capacity(const candy_array_t *self) {
  if (_is_const(self))
    return ((candy_static_t *)self)->size;
  return candy_vector_capacity(&((candy_dynamic_t *)self)->vec);
}

size_t candy_array_size(const candy_array_t *self) {
  if (_is_const(self))
    return ((candy_static_t *)self)->size;
  return candy_vector_size(&((candy_dynamic_t *)self)->vec);
}

void *candy_array_data(const candy_array_t *self) {
  if (_is_const(self))
    return (void *)((candy_static_t *)self)->data;
  return candy_vector_data(&((candy_dynamic_t *)self)->vec);
}

void candy_array_reserve(candy_array_t *self, candy_gc_t *gc, candy_excep_t *ctx, size_t capacity) {
  arr_assert(!_is_const(self), "object must be a dynamic array");
  candy_vector_reserve(&((candy_dynamic_t *)self)->vec,
    candy_gc_memory(gc), ctx, capacity,
    candy_type_size(candy_object_type((candy_object_t *)self))
  );
}

void candy_array_resize(candy_array_t *self, candy_gc_t *gc, candy_excep_t *ctx, size_t size) {
  arr_assert(!_is_const(self), "object must be a dynamic array");
  candy_vector_resize(&((candy_dynamic_t *)self)->vec,
    candy_gc_memory(gc), ctx, size,
    candy_type_size(candy_object_type((candy_object_t *)self))
  );
}

candy_err_t candy_array_append(candy_array_t *self, candy_gc_t *gc, candy_excep_t *ctx, const void *data, size_t size) {
  arr_assert(!_is_const(self), "object must be a dynamic array");
  return candy_vector_append(&((candy_dynamic_t *)self)->vec,
    candy_gc_memory(gc), ctx, data, size,
    candy_type_size(candy_object_type((candy_object_t *)self))
  );
}
