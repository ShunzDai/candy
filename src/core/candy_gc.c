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
#include "core/candy_object.h"
#include <assert.h>

static candy_object_t *_add_node(candy_gc_t *self, candy_excep_t *ctx, candy_object_t **pos, candy_types_t type, size_t size) {
  candy_object_t *obj = (candy_object_t *)candy_memory_alloc(candy_gc_memory(self), ctx, size);
  candy_object_set_next(obj, *pos);
  candy_object_set_type(obj, type);
  candy_object_set_mask(obj, MASK_NONE);
  candy_object_set_mark(obj, MARK_WHITE);
  *pos = obj;
  return obj;
}

static void _del_node(candy_gc_t *self, candy_object_t **pos) {
  candy_object_t *obj = *pos;
  *pos = *candy_object_next(obj);
  int res = candy_gc_event_handler(self)(obj, self, EVT_DELETE, NULL);
  assert(res >= 0);
}

static candy_err_t _fsm_begin(candy_gc_t *self) {
  int res = candy_gc_event_handler(self)(self->prim, self, EVT_COLOR, NULL);
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
  for (candy_object_t **it = &self->pool; *it; ) {
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

candy_err_t candy_gc_init(candy_gc_t *self, candy_handler_t handler, candy_allocator_t alloc, void *arg) {
  candy_memory_init(&self->mem, alloc, arg);
  self->handler = handler;
  self->fsm = GC_FSM_BEGIN;
  self->pool = NULL;
  self->gray = NULL;
  self->prim = NULL;
  return CANDY_OK;
}

candy_err_t candy_gc_deinit(candy_gc_t *self) {
  while (self->pool)
    _del_node(self, &self->pool);
  if (self->prim)
    candy_gc_event_handler(self)(self->prim, self, EVT_DELETE, NULL);
  candy_memory_deinit(candy_gc_memory(self));
  return CANDY_OK;
}

candy_object_t *candy_gc_add(candy_gc_t *self, candy_excep_t *ctx, candy_types_t type, size_t size) {
  return _add_node(self, ctx, &self->pool, type, size);
}

candy_err_t candy_gc_move(candy_gc_t *self, candy_gc_move_t type) {
  candy_object_t *obj = self->pool;
  self->pool = *candy_object_next(obj);
  candy_object_set_next(obj, NULL);
  switch (type) {
    case GC_MV_PRIM:
      self->prim = obj;
      break;
    default:
      return CANDY_ERR;
  }
  return CANDY_OK;
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
