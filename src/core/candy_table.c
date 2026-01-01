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
#include "core/candy_table.h"
#include "core/candy_lib.h"
#include "core/candy_map.h"
#include "core/candy_object.h"
#include "core/candy_gc.h"
#include "core/candy_wrap.h"
#include "core/candy_vector.h"

typedef struct candy_pair candy_pair_t;

struct candy_pair {
  candy_wrap_t key;
  candy_wrap_t val;
};

struct candy_table {
  candy_object_t header;
  candy_pair_t *data;
  uint8_t cap;
};

// static const char TAG[] = "table";

static candy_hash_t _hash(const void *pos, candy_gc_t *gc) {
  const candy_pair_t *self = (const candy_pair_t *)pos;
  return candy_wrap_hash(&self->key, gc);
}

static bool _is_null(const void *pos) {
  const candy_pair_t *self = (const candy_pair_t *)pos;
  return candy_wrap_type(&self->key) == CANDY_TYPE_NULL;
}

static bool _is_tomb(const void *pos) {
  const candy_pair_t *self = (const candy_pair_t *)pos;
  return candy_wrap_mask(&self->key) & MASK_TOMB;
}

static bool _comp(const void *pos, const void *key, candy_gc_t *gc) {
  const candy_pair_t *self = (const candy_pair_t *)pos;
  const candy_wrap_t *k = (const candy_wrap_t *)key;
  return candy_wrap_hash(&self->key, gc) == candy_wrap_hash(k, gc);
}

static candy_err_t _map_init(candy_map_t *self, void *data, uint8_t cap) {
  candy_err_t err = CANDY_OK;
  self->is_null = _is_null;
  self->is_tomb = _is_tomb;
  self->comp = _comp;
  self->cell = sizeof(candy_pair_t);
  self->cap = cap;
  self->data = data;
  return err;
}

static candy_pair_t *_find(const candy_table_t *self, candy_gc_t *gc, const candy_wrap_t *key, bool expand) {
  candy_hash_t hash = candy_wrap_hash(key, gc);
  candy_map_t map;
  _map_init(&map, self->data, self->cap);
  candy_pair_t *val = (candy_pair_t *)candy_map_find(&map, gc, key, hash, expand);
  return val;
}

static candy_err_t _resize(candy_table_t *self, candy_gc_t *gc, candy_excep_t *ctx, size_t cap) {
  candy_err_t err = CANDY_OK;
  candy_map_t map;
  _map_init(&map, self->data, self->cap);
  err = candy_map_resize(&map, gc, ctx, cap, _hash);
  self->cap = map.cap;
  self->data = (candy_pair_t *)map.data;
  return err;
}

static candy_err_t _table_delete(candy_table_t *self, candy_gc_t *gc, void *arg) {
  candy_gc_free(gc, self->data, sizeof(candy_pair_t) * capacity_to_size(self->cap));
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
  self->data = NULL;
  self->cap = 0;
  return self;
}

candy_table_t *candy_table_create_global(candy_gc_t *gc, candy_excep_t *ctx) {
  candy_table_t *self = (candy_table_t *)candy_gc_add_global(gc, ctx, sizeof(candy_table_t));
  self->data = NULL;
  self->cap = 0;
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

candy_err_t candy_table_fprint(const candy_table_t *self, candy_gc_t *gc, FILE *out) {
  fprintf(out, "\033[1;35m>>> table %p head\033[0m\n", self);
  fprintf(out, "pos  key-type         key-val  val-type         val-val\n");
  for (candy_pair_t *pos = self->data; pos < self->data + capacity_to_size(self->cap); ++pos) {
    fprintf(out, "%3" PRIdPTR, pos - (candy_pair_t *)self->data);
    fprintf(out, "%10s", candy_type_str(candy_wrap_type(&pos->key)));
    candy_wrap_fprint(&pos->key, gc, out);
    fprintf(out, "%10s", candy_type_str(candy_wrap_type(&pos->val)));
    candy_wrap_fprint(&pos->val, gc, out);
    fprintf(out, "\n");
  }
  fprintf(out, "\033[1;35m<<< table %p tail\033[0m\n", self);
  return CANDY_OK;
}

candy_err_t candy_table_resize(candy_table_t *self, candy_gc_t *gc, candy_excep_t *ctx, size_t cap) {
  return _resize(self, gc, ctx, cap);
}

const candy_wrap_t *candy_table_get(const candy_table_t *self, candy_gc_t *gc, const candy_wrap_t *key) {
  const candy_pair_t *pos = _find(self, gc, key, false);
  return pos ? &pos->val : &CANDY_WRAP_NULL;
}

candy_err_t candy_table_set(candy_table_t *self, candy_gc_t *gc, candy_excep_t *ctx, const candy_wrap_t *key, const candy_wrap_t *val) {
  while (1) {
    candy_pair_t *pos = _find(self, gc, key, true);
    if (pos) {
      pos->key = *key;
      pos->val = *val;
      return CANDY_OK;
    }
    else if (self->cap + 1 == 32) {
      return CANDY_ERR_LIMITED;
    }
    _resize(self, gc, ctx, self->cap + 1);
  }
}

candy_err_t candy_table_reset(candy_table_t *self, candy_gc_t *gc, const candy_wrap_t *key) {
  candy_pair_t *pos = _find(self, gc, key, false);
  if (pos) {
    candy_wrap_set_type(&pos->key, CANDY_TYPE_NONE);
    candy_wrap_set_mask(&pos->key, MASK_TOMB);
  }
  return CANDY_OK;
}
