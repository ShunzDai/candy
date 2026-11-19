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
#include "core/candy_hashmap.h"
#include "core/candy_object.h"
#include "core/candy_gc.h"
#include "core/candy_wrap.h"

typedef struct candy_pair candy_pair_t;

struct candy_pair {
  candy_wrap_t key;
  candy_wrap_t val;
};

struct candy_table {
  candy_object_t header;
  candy_object_t *gray;
  candy_pair_t *data;
  size_t used;
  uint8_t cap;
};

/* A free slot is uninitialised memory (`none`), a tombstone is the zeroed
 * `null` slot left by a reset; a real `null` key carries MASK_CONST. */
static bool _is_empty(const void *cell) {
  const candy_pair_t *self = (const candy_pair_t *)cell;
  /* A slot that was never written: zeroed, so type NONE and mask NONE. */
  return candy_wrap_type(&self->key) == CANDY_TYPE_NONE && candy_wrap_mask(&self->key) == MASK_NONE;
}

static bool _is_tomb(const void *cell) {
  const candy_pair_t *self = (const candy_pair_t *)cell;
  return candy_wrap_type(&self->key) == CANDY_TYPE_NONE && candy_wrap_mask(&self->key) == MASK_TOMB;
}

static bool _is_equal(const void *cell, const void *key, candy_gc_t *gc) {
  const candy_pair_t *self = (const candy_pair_t *)cell;
  return candy_wrap_equal(&self->key, (const candy_wrap_t *)key);
}

static candy_hash_t _hash(const void *cell, candy_gc_t *gc) {
  const candy_pair_t *self = (const candy_pair_t *)cell;
  return candy_wrap_hash(&self->key);
}

static const candy_hash_ops_t _ops = {
  .is_empty = _is_empty,
  .is_tomb = _is_tomb,
  .is_equal = _is_equal,
  .hash = _hash,
};

static CANDY_FORCE_INLINE candy_pair_t *_find(const candy_table_t *self, candy_gc_t *gc, const candy_wrap_t *key, candy_hash_t hash, bool expand) {
  return (candy_pair_t *)candy_hashmap_find(self->data, capacity_to_size(self->cap), sizeof(candy_pair_t), &_ops, key, hash, expand, gc);
}

static CANDY_FORCE_INLINE candy_err_t _resize(candy_table_t *self, candy_gc_t *gc, candy_excep_t *ctx, size_t cap) {
  size_t old_size = capacity_to_size(self->cap);
  size_t new_size = capacity_to_size(cap);
  self->data = (candy_pair_t *)candy_hashmap_resize(self->data, old_size, new_size,
    sizeof(candy_pair_t), &_ops, candy_gc_memory(gc), gc, ctx);
  self->cap = (uint8_t)cap;
  return CANDY_OK;
}

static candy_err_t _table_delete(candy_table_t *self, candy_gc_t *gc, void *arg) {
  candy_hashmap_deinit(self->data, capacity_to_size(self->cap), sizeof(candy_pair_t), candy_gc_memory(gc));
  candy_gc_free(gc, self, sizeof(candy_table_t));
  return CANDY_OK;
}

static candy_err_t _table_color(candy_table_t *self, candy_gc_t *gc, void *arg) {
  self->gray = candy_gc_gray_swap(gc, (candy_object_t *)self);
  candy_object_set_mark((candy_object_t *)self, MARK_GRAY);
  return CANDY_OK;
}

static candy_err_t _table_diffuse(candy_table_t *self, candy_gc_t *gc, void *arg) {
  candy_gc_gray_swap(gc, self->gray);
  candy_object_set_mark((candy_object_t *)self, MARK_DARK);
  for (size_t index = 0; index < capacity_to_size(self->cap); ++index) {
    const candy_pair_t *pair = self->data + index;
    if (_is_empty(pair) || _is_tomb(pair))
      continue;
    candy_err_t err = candy_wrap_mark(&pair->key, gc);
    if (err == CANDY_OK)
      err = candy_wrap_mark(&pair->val, gc);
    if (err != CANDY_OK)
      return err;
  }
  return CANDY_OK;
}

candy_table_t *candy_table_create(candy_gc_t *gc, candy_excep_t *ctx) {
  candy_table_t *self = (candy_table_t *)candy_gc_add(gc, ctx, CANDY_TYPE_TABLE, sizeof(candy_table_t));
  self->data = NULL;
  self->used = 0;
  self->cap = 0;
  return self;
}

candy_table_t *candy_table_create_global(candy_gc_t *gc, candy_excep_t *ctx) {
  candy_table_t *self = (candy_table_t *)candy_gc_add_global(gc, ctx, sizeof(candy_table_t));
  self->data = NULL;
  self->used = 0;
  self->cap = 0;
  return self;
}

static void _fprint_indent(FILE *out, size_t level) {
  for (size_t i = 0; i < level; ++i)
    fputs("  ", out);
}

/* Scalars spell themselves; strings and tables are asked through EVT_FORMAT so
 * the container never needs their layout. */
static void _fprint_value(candy_gc_t *gc, const candy_format_t *fmt, const candy_wrap_t *wrap) {
  candy_types_t type = candy_wrap_type(wrap);
  bool object = candy_wrap_mask(wrap) & MASK_OBJECT;
  if (object) {
    const candy_format_t nested = {.out = fmt->out, .quoted = true, .level = fmt->level + 1};
    candy_gc_event_handler(gc)(candy_wrap_get_object(wrap), gc, EVT_FORMAT, (void *)&nested);
    return;
  }
  switch (type) {
    case CANDY_TYPE_NULL:
    case CANDY_TYPE_NONE:
      fputs("null", fmt->out);
      break;
    case CANDY_TYPE_INTEGER:
      fprintf(fmt->out, "%" PRId64, candy_wrap_get_integer(wrap));
      break;
    case CANDY_TYPE_FLOAT:
      fprintf(fmt->out, "%g", candy_wrap_get_float(wrap));
      break;
    case CANDY_TYPE_BOOLEAN:
      fputs(candy_wrap_get_boolean(wrap) ? "true" : "false", fmt->out);
      break;
    case CANDY_TYPE_CHAR:
      fprintf(fmt->out, "<char>");
      break;
    default:
      fprintf(fmt->out, "<%s>", candy_type_str(type));
      break;
  }
}

static void _fprint_key(candy_gc_t *gc, const candy_format_t *fmt, const candy_wrap_t *key) {
  if (candy_wrap_type(key) == CANDY_TYPE_CHAR) {
    const candy_format_t quoted = {.out = fmt->out, .quoted = true, .level = fmt->level + 1};
    candy_wrap_format(key, gc, &quoted);
    return;
  }
  /* JSON keys are strings; spell non-string keys textually. */
  fputc('"', fmt->out);
  _fprint_value(gc, fmt, key);
  fputc('"', fmt->out);
}

static candy_err_t _table_fprint_at(const candy_table_t *self, candy_gc_t *gc, const candy_format_t *fmt) {
  FILE *out = fmt->out;
  size_t level = fmt->level;
  /* An empty table is spelled inline; it has no keys to tell array from map. */
  if (self->used == 0) {
    fputs("[]", out);
    return CANDY_OK;
  }
  size_t length = candy_table_size(self, gc);
  bool array = length == self->used;
  fputs(array ? "[\n" : "{\n", out);
  if (array) {
    for (size_t i = 0; i < length; ++i) {
      candy_wrap_t key;
      candy_wrap_set_integer(&key, (candy_integer_t)i);
      _fprint_indent(out, level + 1);
      _fprint_value(gc, fmt, candy_table_get(self, gc, &key));
      if (i + 1 < length)
        fputc(',', out);
      fputc('\n', out);
    }
  }
  else {
    size_t size = capacity_to_size(self->cap);
    size_t remaining = self->used;
    for (size_t i = 0; i < size; ++i) {
      const candy_pair_t *pos = self->data + i;
      if (_is_empty(pos) || _is_tomb(pos))
        continue;
      _fprint_indent(out, level + 1);
      _fprint_key(gc, fmt, &pos->key);
      fputs(": ", out);
      _fprint_value(gc, fmt, &pos->val);
      if (--remaining != 0)
        fputc(',', out);
      fputc('\n', out);
    }
  }
  _fprint_indent(out, level);
  fputs(array ? "]" : "}", out);
  return CANDY_OK;
}

candy_err_t candy_table_handler(candy_table_t *self, candy_gc_t *gc, candy_events_t evt, void *arg) {
  switch (evt) {
    case EVT_DELETE:  return _table_delete(self, gc, arg);
    case EVT_COLOR:   return _table_color(self, gc, arg);
    case EVT_DIFFUSE: return _table_diffuse(self, gc, arg);
    case EVT_FORMAT:  return _table_fprint_at(self, gc, (const candy_format_t *)arg);
    default:          return CANDY_ERR;
  }
}

candy_err_t candy_table_resize(candy_table_t *self, candy_gc_t *gc, candy_excep_t *ctx, size_t cap) {
  if (capacity_to_size(cap) < self->used)
    return CANDY_ERR_LIMITED;
  return _resize(self, gc, ctx, cap);
}

CANDY_LARGE_LTO_INLINE const candy_wrap_t *candy_table_get(const candy_table_t *self, candy_gc_t *gc, const candy_wrap_t *key) {
  candy_hash_t hash = candy_wrap_hash(key);
  const candy_pair_t *pos = _find(self, gc, key, hash, false);
  return pos ? &pos->val : &CANDY_WRAP_NULL;
}

CANDY_LARGE_LTO_INLINE candy_err_t candy_table_set(candy_table_t *self, candy_gc_t *gc, candy_excep_t *ctx, const candy_wrap_t *key, const candy_wrap_t *val) {
  candy_hash_t hash = candy_wrap_hash(key);
  size_t size = capacity_to_size(self->cap);
  candy_pair_t *pos = _find(self, gc, key, hash, false);
  if (pos) {
    /* A hit already stores this exact key and hash: only the value changes. */
    pos->val = *val;
    return CANDY_OK;
  }
  if (candy_hashmap_grow(self->used, size)) {
    if (self->cap >= 30)
      return CANDY_ERR_LIMITED;
    candy_err_t err = candy_table_resize(self, gc, ctx, self->cap + 1);
    if (err != CANDY_OK)
      return err;
    size = capacity_to_size(self->cap);
  }
  pos = _find(self, gc, key, hash, true);
  if (pos == NULL)
    return CANDY_ERR_LIMITED;
  pos->key = *key;
  pos->val = *val;
  ++self->used;
  return CANDY_OK;
}

candy_err_t candy_table_reset(candy_table_t *self, candy_gc_t *gc, const candy_wrap_t *key) {
  candy_hash_t hash = candy_wrap_hash(key);
  candy_pair_t *pos = _find(self, gc, key, hash, false);
  if (pos) {
    /* Mark the slot as a tombstone: still NONE-typed, but with MASK_OBJECT set
     * so it differs from both an unused slot and a live object key. */
    candy_wrap_set_type(&pos->key, CANDY_TYPE_NONE);
    candy_wrap_set_mask(&pos->key, MASK_TOMB);
    --self->used;
  }
  return CANDY_OK;
}

/* Length of the dense 0..n-1 prefix. The table stores no derived size, so this
 * probes keys instead of trusting an invariant that every write must maintain. */
size_t candy_table_size(const candy_table_t *self, candy_gc_t *gc) {
  size_t size = 0;
  while (size < self->used) {
    candy_wrap_t key;
    candy_wrap_set_integer(&key, (candy_integer_t)size);
    if (_find(self, gc, &key, candy_wrap_hash(&key), false) == NULL)
      break;
    ++size;
  }
  return size;
}
