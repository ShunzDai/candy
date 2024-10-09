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
#include "core/candy_table.h"
#include "core/candy_lib.h"
#include "core/candy_object.h"
#include "core/candy_wrap.h"
#include "core/candy_vector.h"
#include "core/candy_gc.h"
#include <string.h>

typedef struct candy_pair candy_pair_t;

struct candy_pair {
  candy_wrap_t key;
  candy_wrap_t val;
};

struct candy_table {
  candy_object_t header;
  candy_vector_t vec;
};

static  size_t _capacity(const candy_vector_t *self) {
  return candy_vector_capacity(self);
}

static inline candy_pair_t *_head(const candy_vector_t *self) {
  return (candy_pair_t *)candy_vector_data(self);
}

static inline candy_pair_t *_tail(const candy_vector_t *self) {
  return _head(self) + _capacity(self) - 1;
}

static int32_t _next(size_t idx) {
  return (int32_t[]){0, 2, -4, 6, -8, INT32_MAX}[idx];
}

static inline bool _boundary_check(const candy_vector_t *self, candy_pair_t *pair) {
  return (size_t)(pair - _head(self)) < _capacity(self);
}

// static inline size_t hash_string(const char str[], size_t size) {
//   size_t hash = size;
//   size_t step = (size >> 5) + 1;
//   for (size_t i = size; i >= step; i -= step)
//     hash = hash ^ ((hash << 5) + (hash >> 2) + (size_t)str[i - 1]);
//   return hash;
// }

static size_t _hash(const candy_wrap_t *key) {
  switch (candy_wrap_get_type(key)) {
    case CANDY_TYPE_INTEGER:
      return djb_hash(candy_wrap_data(key), 8);
    case CANDY_TYPE_CHAR:
      return 0;
    default:
      return 0;
  }
}

static inline candy_pair_t *_main_position(const candy_vector_t *self, const candy_wrap_t *key) {
  return _head(self) + _hash(key) % _capacity(self);
}

static bool _equal(const candy_wrap_t *keyl, const candy_wrap_t *keyr) {
  if (keyl->type != keyr->type)
    return false;
  switch (candy_wrap_get_type(keyl)) {
    case CANDY_TYPE_INTEGER:
      return candy_wrap_get_integer(keyl) == candy_wrap_get_integer(keyr);
    case CANDY_TYPE_FLOAT:
      return candy_wrap_get_float(keyl) == candy_wrap_get_float(keyr);
    default:
      return false;
  }
}

static int _set(const candy_vector_t *self, const candy_wrap_t *key, const candy_wrap_t *val) {
  candy_pair_t *pair = _main_position(self, key);
  for (size_t idx = 0; _boundary_check(self, pair); pair += _next(++idx)) {
    switch (candy_wrap_get_type(&pair->key)) {
      case CANDY_TYPE_NULL:
        pair->key = *key;
        break;
      default:
        if (_equal(&pair->key, key))
          break;
        continue;
    }
    pair->val = *val;
    return 0;
  }
  return -1;
}

static int _resize(candy_table_t *self, candy_gc_t *gc, candy_exce_t *ctx) {
  candy_vector_t vec;
  candy_vector_init(&vec, sizeof(struct candy_wrap[2]));
  candy_vector_reserve(&vec, candy_gc_memory(gc), ctx, _capacity(&self->vec) + 8);
  begin:
  memset(candy_vector_data(&vec), 0, sizeof(struct candy_wrap[2]) * candy_vector_capacity(&vec));
  for (candy_pair_t *pair = _head(&self->vec); pair <= _tail(&self->vec); ++pair) {
    if (candy_wrap_get_type(&pair->key) == CANDY_TYPE_NULL)
      continue;
    if (_set(&vec, &pair->key, &pair->val) < 0) {
      candy_vector_reserve(&vec, candy_gc_memory(gc), ctx, candy_vector_capacity(&vec) + 8);
      goto begin;
    }
  }
  candy_vector_deinit(&self->vec, candy_gc_memory(gc));
  self->vec = vec;
  return 0;
}

candy_table_t *candy_table_create(candy_gc_t *gc, candy_exce_t *ctx) {
  candy_table_t *self = (candy_table_t *)candy_gc_add(gc, ctx, CANDY_TYPE_TABLE, sizeof(struct candy_table));
  candy_vector_init(&self->vec, sizeof(struct candy_wrap[2]));
  candy_vector_reserve(&self->vec, candy_gc_memory(gc), ctx, 8);
  memset(_head(&self->vec), 0, sizeof(struct candy_wrap[2]) * _capacity(&self->vec));
  return self;
}

int candy_table_delete(candy_table_t *self, candy_gc_t *gc) {
  candy_vector_deinit(&self->vec, candy_gc_memory(gc));
  candy_gc_free(gc, self, sizeof(struct candy_table));
  return 0;
}

int candy_table_fprint(const candy_table_t *self, FILE *out) {
  fprintf(out, "\033[1;35m>>> table %p head\033[0m\n", self);
  fprintf(out, "pos  key-type         key-val  val-type         val-val\n");
  for (candy_pair_t *pair = _head(&self->vec); pair <= _tail(&self->vec); ++pair) {
    fprintf(out, "%3ld", pair - _head(&self->vec));
    fprintf(out, "%10s", candy_type_str(candy_wrap_get_type(&pair->key)));
    candy_wrap_fprint(&pair->key, out, 16);
    fprintf(out, "%10s", candy_type_str(candy_wrap_get_type(&pair->val)));
    candy_wrap_fprint(&pair->val, out, 16);
    fprintf(out, "\n");
  }
  fprintf(out, "\033[1;35m<<< table %p tail\033[0m\n", self);
  return 0;
}

const candy_wrap_t *candy_table_get(const candy_table_t *self, const candy_wrap_t *key) {
  candy_pair_t *pair = _main_position(&self->vec, key);
  for (size_t idx = 0; _boundary_check(&self->vec, pair); pair += _next(++idx)) {
    if (_equal(&pair->key, key))
      return &pair->val;
    else if (candy_wrap_get_type(&pair->key) == CANDY_TYPE_NULL)
      break;
  }
  return &CANDY_WRAP_NULL;
}

int candy_table_set(candy_table_t *self, candy_gc_t *gc, candy_exce_t *ctx, const candy_wrap_t *key, const candy_wrap_t *val) {
  while (_set(&self->vec, key, val) < 0)
    _resize(self, gc, ctx);
  return 0;
}
