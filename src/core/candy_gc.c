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
#include "core/candy_gc.h"
#include "core/candy_map.h"
#include "core/candy_lib.h"
#include "core/candy_object.h"
#include <string.h>
#include <assert.h>

struct map_key {
  candy_types_t type;
  const void *data;
  size_t size;
  candy_hash_t hash;
};

static const char TAG[] = "gc";

static candy_hash_t _hash(const void *pos, candy_gc_t *gc) {
  candy_object_t **self = (candy_object_t **)pos;
  candy_hash_t hash = 0;
  candy_gc_event_handler(gc)(*self, gc, EVT_HASH, &hash);
  return hash;
}

static bool _is_null(const void *pos) {
  const candy_object_t **self = (const candy_object_t **)pos;
  return *self == NULL || *self == (candy_object_t *)UINTPTR_MAX;
}

static bool _is_tomb(const void *pos) {
  const candy_object_t **self = (const candy_object_t **)pos;
  return *self == (candy_object_t *)UINTPTR_MAX;
}

static bool _comp(const void *pos, const void *key, candy_gc_t *gc) {
  candy_object_t **self = (candy_object_t **)pos;
  struct map_key *k = (struct map_key *)key;
  bool val = true;
  if (candy_object_type(*self) != k->type) {
    val = false;
  }
  else if (_hash(self, gc) != k->hash) {
    val = false;
  }
  else if (candy_gc_event_handler(gc)(*self, gc, EVT_COMPARE, k) != CANDY_OK) {
    val = false;
  }
  return val;
}

static candy_err_t _map_init(candy_map_t *self, void *data, uint8_t cap) {
  candy_err_t err = CANDY_OK;
  self->is_null = _is_null;
  self->is_tomb = _is_tomb;
  self->comp = _comp;
  self->cell = sizeof(candy_object_t *);
  self->cap = cap;
  self->data = data;
  return err;
}

static candy_object_t **_find(candy_gc_t *self, const struct map_key *key, candy_hash_t hash, bool expand) {
  candy_map_t map;
  _map_init(&map, self->pool, self->pool_cap);
  return (candy_object_t **)candy_map_find(&map, self, key, hash, expand);
}

static candy_err_t _resize(candy_gc_t *self, candy_excep_t *ctx, size_t cap) {
  candy_err_t err = CANDY_OK;
  candy_map_t map;
  _map_init(&map, self->pool, self->pool_cap);
  err = candy_map_resize(&map, self, ctx, cap, _hash);
  self->pool_cap = map.cap;
  self->pool = (candy_object_t **)map.data;
  return err;
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
  int res = candy_gc_event_handler(self)(obj, self, EVT_DELETE, NULL);
  assert(res >= 0);
}

static candy_err_t _fsm_begin(candy_gc_t *self) {
  int res = candy_gc_event_handler(self)((candy_object_t *)self->prim, self, EVT_COLOR, NULL);
  assert(res >= 0);
  return CANDY_OK;
}

static candy_err_t _fsm_diffuse(candy_gc_t *self) {
  candy_object_t *obj = self->gray;
  /* remove from 'gray' list */
  int res = candy_gc_event_handler(self)(obj, self, EVT_DIFFUSE, NULL);
  assert(res >= 0);
  return CANDY_OK;
}

static candy_err_t _fsm_end(candy_gc_t *self) {
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
  return CANDY_OK;
}

candy_err_t candy_gc_init(candy_gc_t *self, candy_excep_t *ctx, candy_handler_t handler, candy_allocator_t alloc, void *arg) {
  candy_memory_init(&self->mem, alloc, arg);
  self->pool = NULL;
  self->pool_cap = 0;
  self->handler = handler;
  self->fsm = GC_FSM_BEGIN;
  self->list = NULL;
  self->gray = NULL;
  self->prim = NULL;
  return CANDY_OK;
}

candy_err_t candy_gc_deinit(candy_gc_t *self) {
  /* free constant pool */
  for (size_t idx = 0; idx < capacity_to_size(self->pool_cap); ++idx) {
    candy_object_t **pos = (candy_object_t **)self->pool + idx;
    if (*pos == NULL || *pos == (candy_object_t *)UINTPTR_MAX)
      continue;
    _del_node(self, pos);
  }
  candy_memory_free(candy_gc_memory(self), self->pool, sizeof(candy_object_t *) * capacity_to_size(self->pool_cap));
  /* free object list */
  while (self->list)
    _del_node(self, &self->list);
  /* free global table */
  if (self->glob)
    candy_gc_event_handler(self)((candy_object_t *)self->glob, self, EVT_DELETE, NULL);
  /* free primary state */
  if (self->prim)
    candy_gc_event_handler(self)((candy_object_t *)self->prim, self, EVT_DELETE, NULL);
  /* deinitialize memory module */
  candy_memory_deinit(candy_gc_memory(self));
  return CANDY_OK;
}

candy_object_t *candy_gc_add(candy_gc_t *self, candy_excep_t *ctx, candy_types_t type, size_t size) {
  return _add_node(self, ctx, &self->list, type, size);
}

candy_object_t *candy_gc_add_pool(candy_gc_t *self, candy_excep_t *ctx, candy_types_t type, size_t size, candy_hash_t hash) {
  struct map_key key = {
    .type = type,
    .data = NULL,
    .size = size,
    .hash = hash,
  };
  while (1) {
    candy_object_t **pos = _find(self, &key, hash, true);
    if (pos) {
      return _add_node(self, ctx, pos, type, size);
    }
    else if (self->pool_cap + 1 == 32) {
      return NULL;
    }
    _resize(self, ctx, self->pool_cap + 1);
  }
}

candy_state_t *candy_gc_add_primary(candy_gc_t *self, candy_excep_t *ctx, size_t size) {
  return (candy_state_t *)_add_node(self, ctx, (candy_object_t **)&self->prim, CANDY_TYPE_STATE, size);
}

candy_table_t *candy_gc_add_global(candy_gc_t *self, candy_excep_t *ctx, size_t size) {
  return (candy_table_t *)_add_node(self, ctx, (candy_object_t **)&self->glob, CANDY_TYPE_TABLE, size);
}

candy_object_t *candy_gc_find(candy_gc_t *self, candy_types_t type, const void *data, size_t size, candy_hash_t hash) {
  struct map_key key = {
    .type = type,
    .data = data,
    .size = size,
    .hash = hash,
  };
  candy_object_t **pos = _find(self, &key, hash, false);
  return pos ? *pos : NULL;
}

candy_err_t candy_gc_sweep(candy_gc_t *self) {
  // for (candy_object_t *obj = self->root, *next = candy_object_next(obj); obj;) {
  //   if (candy_object_mark(obj) == MARK_DARK)

  // }
  return CANDY_OK;
}

candy_err_t candy_gc_step(candy_gc_t *self) {
  switch (self->fsm) {
    case GC_FSM_BEGIN:
      _fsm_begin(self);
      self->fsm = GC_FSM_DIFFUSE;
      return CANDY_OK;
    case GC_FSM_DIFFUSE:
      if (self->gray)
        _fsm_diffuse(self);
      else
        self->fsm = GC_FSM_END;
      return CANDY_OK;
    case GC_FSM_END:
      _fsm_end(self);
      self->fsm = GC_FSM_BEGIN;
      return CANDY_OK;
    default:
      return CANDY_ERR;
  }
}

candy_err_t candy_gc_full(candy_gc_t *self) {
  if (candy_gc_fsm(self) == GC_FSM_BEGIN)
    candy_gc_step(self);
  while (candy_gc_fsm(self) != GC_FSM_BEGIN)
    candy_gc_step(self);
  return CANDY_OK;
}
