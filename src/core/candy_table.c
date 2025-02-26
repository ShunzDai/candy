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
#include "core/candy_table.h"
#include "core/candy_object.h"
#include "core/candy_wrap.h"
#include "core/candy_vector.h"
#include "core/candy_gc.h"

typedef struct candy_pair candy_pair_t;

struct candy_pair {
  candy_wrap_t key;
  candy_wrap_t val;
};

struct candy_table {
  candy_object_t header;
  uint8_t cap;
  candy_pair_t *data;
};

static inline int32_t _next(size_t idx) {
  return (int32_t[]){3, -3, 5, -5, 7, -7, INT32_MAX}[idx];
}

static inline size_t _size(uint8_t cap) {
  return cap ? 1 << cap : 0;
}

static candy_hash_t _hash(const candy_wrap_t *pos) {
  switch (candy_wrap_type(pos)) {
    case CANDY_TYPE_INTEGER:
      return candy_wrap_get_integer(pos);
    case CANDY_TYPE_CHAR:
      return 0;
    default:
      return 0;
  }
}

static inline candy_pair_t *_head(const candy_table_t *self) {
  return self->data;
}

static inline candy_pair_t *_position(const candy_table_t *self, candy_hash_t hash) {
  return _head(self) + (hash & (_size(self->cap) - 1));
}

static candy_pair_t *_find(const candy_table_t *self, const candy_wrap_t *key, bool view) {
  candy_hash_t hash = _hash(key);
  for (size_t idx = 0; _next(idx) != INT32_MAX; ++idx) {
    candy_pair_t *pos = _position(self, hash + _next(idx));
    if (candy_wrap_type(&pos->key) == CANDY_TYPE_NONE) {
      if (view) {
        if (candy_wrap_mask(&pos->key) & MASK_TOMB)
          continue;
        break;
      }
    }
    else if (_hash(&pos->key) != hash) {
      continue;
    }
    pos->key = *key;
    return pos;
  }
  return NULL;
}

static candy_err_t _table_delete(candy_table_t *self, candy_gc_t *gc, void *arg) {
  candy_gc_free(gc, self->data, sizeof(candy_pair_t) * _size(self->cap));
  candy_gc_free(gc, self, sizeof(candy_table_t));
  return CANDY_OK;
}

static candy_err_t _table_color(candy_table_t *self, candy_gc_t *gc, void *arg) {
  return CANDY_OK;
}

static candy_err_t _table_diffuse(candy_table_t *self, candy_gc_t *gc, void *arg) {
  return CANDY_OK;
}

candy_table_t *candy_table_create(candy_gc_t *gc, candy_excep_t *ctx) {
  candy_table_t *self = (candy_table_t *)candy_gc_add(gc, ctx, CANDY_TYPE_TABLE, sizeof(candy_table_t));
  self->cap = 0;
  self->data = NULL;
  candy_table_resize(self, gc, ctx, 3);
  return self;
}

candy_err_t candy_table_handler(candy_table_t *self, candy_gc_t *gc, candy_events_t evt, void *arg) {
  switch (evt) {
    case EVT_DELETE:  return _table_delete(self, gc, arg);
    case EVT_COLOR:   return _table_color(self, gc, arg);
    case EVT_DIFFUSE: return _table_diffuse(self, gc, arg);
    default:          return CANDY_ERR;
  }
}

candy_err_t candy_table_fprint(const candy_table_t *self, FILE *out) {
  fprintf(out, "\033[1;35m>>> table %p head\033[0m\n", self);
  fprintf(out, "pos  key-type         key-val  val-type         val-val\n");
  for (candy_pair_t *pos = self->data; pos < self->data + (1 << self->cap); ++pos) {
    fprintf(out, "%3ld", pos - self->data);
    fprintf(out, "%10s", candy_type_str(candy_wrap_type(&pos->key)));
    candy_wrap_fprint(&pos->key, out, 16);
    fprintf(out, "%10s", candy_type_str(candy_wrap_type(&pos->val)));
    candy_wrap_fprint(&pos->val, out, 16);
    fprintf(out, "\n");
  }
  fprintf(out, "\033[1;35m<<< table %p tail\033[0m\n", self);
  return CANDY_OK;
}

candy_err_t candy_table_resize(candy_table_t *self, candy_gc_t *gc, candy_excep_t *ctx, size_t cap) {
  size_t nsize = _size(cap);
  size_t psize = _size(self->cap);
  candy_table_t tb;
  tb.cap = cap;
  tb.data = candy_memory_alloc(candy_gc_memory(gc), ctx, sizeof(candy_pair_t) * nsize);
  memset(_head(&tb), 0, sizeof(candy_pair_t) * nsize);
  for (size_t idx = 0; idx < psize; ++idx) {
    candy_pair_t *from = _head(self) + idx;
    if (candy_wrap_type(&from->key) == CANDY_TYPE_NONE)
      continue;
    candy_pair_t *to = _find(&tb, &from->key, false);
    if (to) {
      to->val = from->val;
    }
  }
  candy_memory_free(candy_gc_memory(gc), self->data, sizeof(candy_pair_t) * psize);
  self->data = tb.data;
  self->cap = tb.cap;
  return CANDY_OK;
}

const candy_wrap_t *candy_table_get(const candy_table_t *self, const candy_wrap_t *key) {
  const candy_pair_t *pos = _find(self, key, true);
  return pos ? &pos->val : &CANDY_WRAP_NULL;
}

candy_err_t candy_table_set(candy_table_t *self, candy_gc_t *gc, candy_excep_t *ctx, const candy_wrap_t *key, const candy_wrap_t *val) {
  while (1) {
    candy_pair_t *pos = _find(self, key, false);
    if (pos) {
      pos->val = *val;
      return CANDY_OK;
    }
    candy_table_resize(self, gc, ctx, self->cap + 1);
  }
}

candy_err_t candy_table_reset(candy_table_t *self, const candy_wrap_t *key) {
  candy_pair_t *pos = _find(self, key, true);
  if (pos) {
    candy_wrap_set_type(&pos->key, CANDY_TYPE_NONE);
    candy_wrap_set_mask(&pos->key, MASK_TOMB);
  }
  return CANDY_OK;
}
