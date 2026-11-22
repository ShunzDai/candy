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
#include "core/candy_gc.h"
#include "core/candy_hashmap.h"
#include "core/candy_lib.h"
#include "core/candy_object.h"
#include <string.h>
#include <assert.h>

struct candy_gc_key {
  candy_types_t type;
  const void *data;
  size_t size;
};

static const char TAG[] = "gc";

static bool _is_empty(const void *cell) {
  const candy_object_t *const *self = (const candy_object_t *const *)cell;
  return *self == NULL;
}

static bool _is_tomb(const void *cell) {
  const candy_object_t *const *self = (const candy_object_t *const *)cell;
  return *self == (candy_object_t *)UINTPTR_MAX;
}

static bool _is_equal(const void *cell, const void *key, candy_gc_t *gc) {
  const candy_object_t *const *self = (const candy_object_t *const *)cell;
  const candy_gc_key_t *k = (const candy_gc_key_t *)key;
  if (candy_object_type(*self) != k->type)
    return false;
  return candy_gc_event_handler(gc)((candy_object_t *)*self, gc, EVT_COMPARE, (void *)k) == CANDY_OK;
}

static candy_hash_t _hash(const void *cell, candy_gc_t *gc) {
  (void)gc;
  /* Pooled objects cache their content hash in the object header. */
  const candy_object_t *const *self = (const candy_object_t *const *)cell;
  return candy_object_hash(*self);
}

static const candy_hash_ops_t _ops = {
  .is_empty = _is_empty,
  .is_tomb = _is_tomb,
  .is_equal = _is_equal,
  .hash = _hash,
};

static candy_object_t **_find(candy_gc_t *self, const candy_gc_key_t *key, candy_hash_t hash, bool expand) {
  return (candy_object_t **)candy_hashmap_find(self->pool,
    capacity_to_size(self->pool_cap), sizeof(candy_object_t *),
    &_ops, key, hash, expand, self
  );
}

static candy_err_t _resize(candy_gc_t *self, candy_excep_t *ctx, size_t cap) {
  size_t old_size = capacity_to_size(self->pool_cap);
  size_t new_size = capacity_to_size(cap);
  self->pool = (candy_object_t **)candy_hashmap_resize(self->pool,
    old_size, new_size, sizeof(candy_object_t *), &_ops, &self->mem, self, ctx
  );
  self->pool_cap = (uint8_t)cap;
  return CANDY_OK;
}

static candy_object_t *_add_node(candy_gc_t *self, candy_excep_t *ctx, candy_object_t **pos, candy_types_t type, size_t size) {
  candy_object_t *obj = (candy_object_t *)candy_memory_alloc(candy_gc_memory(self), ctx, size);
  candy_object_set_next(obj, *pos);
  candy_object_set_type(obj, type);
  candy_object_set_mask(obj, MASK_OBJECT);
  candy_object_set_mark(obj, MARK_WHITE);
  *pos = obj;
  candy_logd(TAG, "add %s at %p", candy_type_str(type), obj);
  return obj;
}

static void _del_node(candy_gc_t *self, candy_object_t **pos) {
  candy_object_t *obj = *pos;
  candy_logd(TAG, "del %s at %p", candy_type_str(candy_object_type(obj)), obj);
  *pos = *candy_object_next(obj);
  candy_gc_event_handler(self)(obj, self, EVT_DELETE, NULL);
}

static candy_err_t _fsm_begin(candy_gc_t *self) {
  candy_logd(TAG, "into %s %s:%d", __FUNCTION__, __FILE__, __LINE__);
  for (candy_object_t *it = self->list; it; it = *candy_object_next(it)) {
    if (!(candy_object_mask(it) & MASK_ROOT))
      continue;
    candy_err_t err = candy_gc_mark(self, it);
    if (err != CANDY_OK)
      return err;
  }
  candy_err_t err = candy_gc_mark(self, self->glob);
  if (err != CANDY_OK)
    return err;
  err = candy_gc_mark(self, (candy_object_t *)self->modules);
  return err;
}

static candy_err_t _fsm_diffuse(candy_gc_t *self) {
  candy_logd(TAG, "into %s %s:%d", __FUNCTION__, __FILE__, __LINE__);
  candy_object_t *obj = self->gray;
  /* remove from 'gray' list */
  return candy_gc_event_handler(self)(obj, self, EVT_DIFFUSE, NULL);
}

static candy_err_t _fsm_end(candy_gc_t *self) {
  candy_logd(TAG, "into %s %s:%d", __FUNCTION__, __FILE__, __LINE__);
  for (candy_object_t **it = &self->list; *it; ) {
    switch (candy_object_mark(*it)) {
      case MARK_WHITE:
        _del_node(self, it);
        break;
      case MARK_DARK:
        candy_object_set_mark(*it, MARK_WHITE);
        it = candy_object_next(*it);
        break;
      default:
        assert(0);
    }
  }
  for (size_t idx = 0; idx < capacity_to_size(self->pool_cap); ++idx) {
    candy_object_t **pos = self->pool + idx;
    if (_is_empty(pos) || _is_tomb(pos))
      continue;
    if (candy_object_mark(*pos) == MARK_WHITE) {
      _del_node(self, pos);
      *pos = (candy_object_t *)UINTPTR_MAX;
      --self->pool_used;
    }
    else {
      candy_object_set_mark(*pos, MARK_WHITE);
    }
  }
  if (self->glob)
    candy_object_set_mark(self->glob, MARK_WHITE);
  return CANDY_OK;
}

candy_err_t candy_gc_init(candy_gc_t *self, candy_handler_t handler, candy_allocator_t alloc, void *arg) {
  candy_memory_init(&self->mem, alloc, arg);
  self->threshold = CANDY_CONFIG_GC_THRESHOLD;
  self->pool = NULL;
  self->pool_used = 0;
  self->pool_cap = 0;
  self->handler = handler;
  self->fsm = GC_FSM_BEGIN;
  self->list = NULL;
  self->gray = NULL;
  self->glob = NULL;
  self->modules = NULL;
  return CANDY_OK;
}

candy_err_t candy_gc_deinit(candy_gc_t *self) {
  /* free constant pool */
  for (size_t idx = 0; idx < capacity_to_size(self->pool_cap); ++idx) {
    candy_object_t **pos = (candy_object_t **)self->pool + idx;
    if (_is_empty(pos) || _is_tomb(pos))
      continue;
    _del_node(self, pos);
  }
  candy_hashmap_deinit(self->pool, capacity_to_size(self->pool_cap), sizeof(candy_object_t *), &self->mem);
  self->pool_used = 0;
  /* free object list */
  while (self->list)
    _del_node(self, &self->list);
  /* free global table */
  if (self->glob)
    candy_gc_event_handler(self)((candy_object_t *)self->glob, self, EVT_DELETE, NULL);
  /* deinitialize memory module */
  candy_memory_deinit(candy_gc_memory(self));
  return CANDY_OK;
}

candy_object_t *candy_gc_add(candy_gc_t *self, candy_excep_t *ctx, candy_types_t type, size_t size) {
  return _add_node(self, ctx, &self->list, type, size);
}

candy_object_t *candy_gc_add_pool(candy_gc_t *self, candy_excep_t *ctx, candy_types_t type, size_t size, candy_hash_t hash) {
  /* Searching for a free slot: there is no payload to compare against. */
  candy_gc_key_t key = {
    .type = type,
    .data = NULL,
    .size = 0,
  };
  size_t pool_size = capacity_to_size(self->pool_cap);
  if (candy_hashmap_grow(self->pool_used, pool_size)) {
    if (self->pool_cap >= 30)
      return NULL;
    /* The pool is append-heavy while parsing: 4x growth cuts rehash count. */
    size_t next_cap = self->pool_cap + 2;
    if (next_cap > 30)
      next_cap = 30;
    candy_err_t err = _resize(self, ctx, next_cap);
    if (err != CANDY_OK)
      return NULL;
    pool_size = capacity_to_size(self->pool_cap);
  }
  candy_object_t **pos = _find(self, &key, hash, true);
  if (pos == NULL)
    return NULL;
  candy_object_t *obj = _add_node(self, ctx, pos, type, size);
  ++self->pool_used;
  return obj;
}

candy_state_t *candy_gc_add_primary(candy_gc_t *self, candy_excep_t *ctx, size_t size) {
  candy_object_t *obj = candy_gc_add(self, ctx, CANDY_TYPE_STATE, size);
  candy_object_set_mask(obj, candy_object_mask(obj) | MASK_ROOT);
  return (candy_state_t *)obj;
}

candy_table_t *candy_gc_add_global(candy_gc_t *self, candy_excep_t *ctx, size_t size) {
  return (candy_table_t *)_add_node(self, ctx, (candy_object_t **)&self->glob, CANDY_TYPE_TABLE, size);
}

candy_object_t *candy_gc_find(candy_gc_t *self, candy_types_t type, const void *data, size_t size, candy_hash_t hash) {
  candy_gc_key_t key = {
    .type = type,
    .data = data,
    .size = size,
  };
  candy_object_t **pos = _find(self, &key, hash, false);
  return pos ? *pos : NULL;
}

candy_err_t candy_gc_mark(candy_gc_t *self, candy_object_t *obj) {
  if (obj == NULL || candy_object_mark(obj) != MARK_WHITE)
    return CANDY_OK;
  return candy_gc_event_handler(self)(obj, self, EVT_COLOR, NULL);
}

const void *candy_gc_key_data(const candy_gc_key_t *self) {
  return self->data;
}

size_t candy_gc_key_size(const candy_gc_key_t *self) {
  return self->size;
}

candy_err_t candy_gc_step(candy_gc_t *self) {
  candy_err_t err = CANDY_OK;
  switch (candy_gc_fsm(self)) {
    case GC_FSM_BEGIN:
      err = _fsm_begin(self);
      if (err == CANDY_OK)
        self->fsm = GC_FSM_DIFFUSE;
      break;
    case GC_FSM_DIFFUSE:
      if (self->gray)
        err = _fsm_diffuse(self);
      else
        self->fsm = GC_FSM_END;
      break;
    case GC_FSM_END:
      _fsm_end(self);
      self->fsm = GC_FSM_BEGIN;
      break;
    default:
      err = CANDY_ERR;
      break;
  }
  return err;
}

candy_err_t candy_gc_full(candy_gc_t *self) {
  do {
    candy_err_t err = candy_gc_step(self);
    if (err != CANDY_OK)
      return err;
  } while (candy_gc_fsm(self) != GC_FSM_BEGIN);
  size_t used = candy_memory_used(candy_gc_memory(self));
  self->threshold = used > SIZE_MAX / 2 ? SIZE_MAX : used * 2;
  if (self->threshold < CANDY_CONFIG_GC_THRESHOLD)
    self->threshold = CANDY_CONFIG_GC_THRESHOLD;
  return CANDY_OK;
}
