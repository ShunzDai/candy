/**
  * Copyright 2022-2026 ShunzDai
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
#include "core/candy_wrap.h"
#include "core/candy_vector.h"
#include <string.h>
#include <stdarg.h>

typedef struct candy_dynamic candy_dynamic_t;
typedef struct candy_static candy_static_t;

struct candy_array {
  candy_object_t header;
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

static inline size_t _type_size(candy_types_t type) {
  static const size_t sizes[] = {
    #define CANDY_TYPE_SIZE
    #include "core/candy_type.list"
  };
  return sizes[type];
}

static inline bool _is_const(const candy_array_t *self) {
  return candy_object_mask((candy_object_t *)self) & MASK_CONST;
}

static candy_err_t _array_delete(candy_array_t *self, candy_gc_t *gc, void *arg) {
  size_t cell = _type_size(candy_object_type((candy_object_t *)self));
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
    case CANDY_TYPE_CCLOS:
    case CANDY_TYPE_SCLOS:
    case CANDY_TYPE_UPVAL:
    case CANDY_TYPE_TABLE:
    case CANDY_TYPE_PROTO:
    case CANDY_TYPE_STATE:
    case CANDY_TYPE_WRAP:
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
  size_t size = candy_array_size(self);
  switch (candy_object_type((candy_object_t *)self)) {
    case CANDY_TYPE_WRAP: {
      const candy_wrap_t *items = (const candy_wrap_t *)candy_array_data(self);
      for (size_t index = 0; index < size; ++index) {
        candy_err_t err = candy_wrap_mark(&items[index], gc);
        if (err != CANDY_OK)
          return err;
      }
      return CANDY_OK;
    }
    case CANDY_TYPE_CCLOS:
    case CANDY_TYPE_SCLOS:
    case CANDY_TYPE_UPVAL:
    case CANDY_TYPE_TABLE:
    case CANDY_TYPE_PROTO:
    case CANDY_TYPE_STATE: {
      candy_object_t *const *items = (candy_object_t *const *)candy_array_data(self);
      for (size_t index = 0; index < size; ++index) {
        candy_err_t err = candy_gc_mark(gc, items[index]);
        if (err != CANDY_OK)
          return err;
      }
      return CANDY_OK;
    }
    default:
      return CANDY_OK;  /* Scalar elements hold no GC references. */
  }
}

/* GC checks the type first; the array checks the element count and bytes. */
static candy_err_t _array_compare(candy_array_t *self, candy_gc_t *gc, void *arg) {
  const candy_static_t *array = (const candy_static_t *)self;
  const candy_gc_key_t *key = (const candy_gc_key_t *)arg;
  const void *data = candy_gc_key_data(key);
  size_t size = candy_gc_key_size(key);
  size_t len;
  /* Only a lookup carrying an actual payload can match. */
  if (data == NULL || size != array->size)
    return CANDY_ERR;
  len = _type_size(candy_object_type((const candy_object_t *)self)) * size;
  return memcmp(array->data, data, len) == 0 ? CANDY_OK : CANDY_ERR;
}

static void _format_quoted(FILE *out, const char data[], size_t size) {
  fputc('"', out);
  for (size_t i = 0; i < size; ++i) {
    char c = data[i];
    switch (c) {
      case '"':  fputs("\\\"", out); break;
      case '\\': fputs("\\\\", out); break;
      case '\n': fputs("\\n", out); break;
      case '\r': fputs("\\r", out); break;
      case '\t': fputs("\\t", out); break;
      default:
        if ((unsigned char)c < 0x20)
          fprintf(out, "\\u%04x", (unsigned char)c);
        else
          fputc(c, out);
    }
  }
  fputc('"', out);
}

static void _format_indent(FILE *out, size_t level) {
  for (size_t index = 0; index < level; ++index)
    fputs("  ", out);
}

/* A value array spells itself like JSON; a string owns its own escaping, so
 * containers never need the array layout. */
static candy_err_t _array_format(candy_array_t *self, candy_gc_t *gc, void *arg) {
  const candy_format_t *fmt = (const candy_format_t *)arg;
  size_t size = candy_array_size(self);
  if (candy_object_type((candy_object_t *)self) == CANDY_TYPE_WRAP) {
    const candy_wrap_t *items = (const candy_wrap_t *)candy_array_data(self);
    fputs(size ? "[\n" : "[]", fmt->out);
    for (size_t index = 0; index < size; ++index) {
      _format_indent(fmt->out, fmt->level + 1);
      const candy_format_t nested = {.out = fmt->out, .quoted = true, .level = fmt->level + 1};
      candy_wrap_format(&items[index], gc, &nested);
      if (index + 1 < size)
        fputc(',', fmt->out);
      fputc('\n', fmt->out);
    }
    if (size) {
      _format_indent(fmt->out, fmt->level);
      fputc(']', fmt->out);
    }
    return CANDY_OK;
  }
  assert(candy_object_type((candy_object_t *)self) == CANDY_TYPE_CHAR);
  const char *data = (const char *)candy_array_data(self);
  if (fmt->quoted)
    _format_quoted(fmt->out, data, size);
  else
    fwrite(data, sizeof(char), size, fmt->out);
  return CANDY_OK;
}

/* Dynamic char arrays are scratch buffers. Script-visible strings are
 * interned through candy_array_create_const, keeping equality by identity. */
candy_array_t *candy_array_create(candy_gc_t *gc, candy_excep_t *ctx, candy_types_t type) {
  candy_dynamic_t *self = (candy_dynamic_t *)candy_gc_add(gc, ctx, type, sizeof(candy_dynamic_t));
  candy_object_set_mask((candy_object_t *)self, candy_object_mask((candy_object_t *)self) | MASK_ARRAY);
  candy_object_set_hash((candy_object_t *)self, 0);
  self->arr.gray = NULL;
  candy_vector_init(&((candy_dynamic_t *)self)->vec);
  return (candy_array_t *)self;
}

static inline candy_array_t *_array_intern(candy_gc_t *gc, candy_excep_t *ctx, candy_types_t type,
                                           const void *data, size_t size, size_t cell, candy_hash_t hash) {
  size_t len = size * cell;
  candy_static_t *self;
  if (data == NULL && len == 0)
    data = "";  /* Give an empty payload a valid pointer for comparison and copy. */
  self = (candy_static_t *)candy_gc_find(gc, type, data, size, hash);
  if (self != NULL)
    return (candy_array_t *)self;  /* Pool hits already match the immutable bytes. */
  self = (candy_static_t *)candy_gc_add_pool(gc, ctx, type, sizeof(candy_static_t) + len + cell, hash);
  candy_object_set_mask((candy_object_t *)self, candy_object_mask((candy_object_t *)self) | MASK_ARRAY | MASK_CONST);
  memcpy(self->data, data, len);
  memset(self->data + len, 0, cell);
  self->size = size;
  candy_object_set_hash((candy_object_t *)self, hash);
  self->arr.gray = NULL;
  return (candy_array_t *)self;
}

static inline size_t _const_array_cell(candy_gc_t *gc, candy_excep_t *ctx, candy_types_t type, size_t size) {
  size_t cell = _type_size(type);
  arr_assert(cell == 0 || size < (SIZE_MAX - sizeof(candy_static_t)) / cell, "array is too large");
  return cell;
}

candy_array_t *candy_array_create_const(candy_gc_t *gc, candy_excep_t *ctx, candy_types_t type, const void *data, size_t size) {
  size_t cell = _const_array_cell(gc, ctx, type, size);
  return _array_intern(gc, ctx, type, data, size, cell, hash_djb(data, size * cell));
}

candy_array_t *candy_array_create_const_hashed(candy_gc_t *gc, candy_excep_t *ctx, candy_types_t type, const void *data, size_t size, candy_hash_t hash) {
  size_t cell = _const_array_cell(gc, ctx, type, size);
  return _array_intern(gc, ctx, type, data, size, cell, hash);
}

candy_array_t *candy_array_vprint(candy_gc_t *gc, candy_excep_t *ctx, const char format[], va_list args) {
  va_list args_copy;
  va_copy(args_copy, args);
  int len = vsnprintf(NULL, 0, format, args_copy);
  candy_array_t *self = candy_array_create(gc, ctx, CANDY_TYPE_CHAR);
  candy_array_resize(self, gc, ctx, len + 1);
  vsnprintf((char *)candy_array_data(self), len + 1, format, args);
  candy_array_resize(self, gc, ctx, len);
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
    case EVT_COMPARE: return _array_compare(self, gc, arg);
    case EVT_FORMAT:  return _array_format(self, gc, arg);
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
    _type_size(candy_object_type((candy_object_t *)self))
  );
}

void candy_array_resize(candy_array_t *self, candy_gc_t *gc, candy_excep_t *ctx, size_t size) {
  arr_assert(!_is_const(self), "object must be a dynamic array");
  candy_vector_resize(&((candy_dynamic_t *)self)->vec,
    candy_gc_memory(gc), ctx, size,
    _type_size(candy_object_type((candy_object_t *)self))
  );
}

candy_err_t candy_array_append(candy_array_t *self, candy_gc_t *gc, candy_excep_t *ctx, const void *data, size_t size) {
  arr_assert(!_is_const(self), "object must be a dynamic array");
  return candy_vector_append(&((candy_dynamic_t *)self)->vec,
    candy_gc_memory(gc), ctx, data, size,
    _type_size(candy_object_type((candy_object_t *)self))
  );
}

const candy_wrap_t *candy_array_get(const candy_array_t *self, size_t index) {
  assert(candy_object_type((const candy_object_t *)self) == CANDY_TYPE_WRAP);
  if (index >= candy_array_size(self))
    return &CANDY_WRAP_NULL;
  return (const candy_wrap_t *)candy_array_data(self) + index;
}

candy_err_t candy_array_set(candy_array_t *self, candy_gc_t *gc, candy_excep_t *ctx, size_t index, const candy_wrap_t *value) {
  arr_assert(candy_object_type((candy_object_t *)self) == CANDY_TYPE_WRAP, "value array expected");
  arr_assert(!_is_const(self), "object must be a dynamic array");
  size_t size = candy_array_size(self);
  if (index == size)
    return candy_array_append(self, gc, ctx, value, 1);
  if (index > size)
    return CANDY_ERR_LIMITED;  /* dense only: no holes */
  ((candy_wrap_t *)candy_array_data(self))[index] = *value;
  return CANDY_OK;
}
