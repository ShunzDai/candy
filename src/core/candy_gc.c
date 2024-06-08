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
#include "core/candy_gc.h"
#include "core/candy_object.h"
#include <assert.h>

static candy_object_t *_add_node(candy_gc_t *self, candy_object_t **pos, candy_types_t type, size_t size) {
  candy_object_t *obj = (candy_object_t *)candy_gc_alloc(self, NULL, 0, size);
  candy_object_set_next(obj, *pos);
  candy_object_set_type(obj, type);
  candy_object_set_mask(obj, MASK_NONE);
  candy_object_set_mark(obj, MARK_WHITE);
  *pos = obj;
  return obj;
}

static void _del_node(candy_gc_t *self, candy_object_t **pos) {
  candy_object_t *obj = *pos;
  *pos = *candy_object_get_next(obj);
  int res = candy_gc_event_handler(self)(obj, self, EVT_DELETE);
  assert(res >= 0);
}

static int _fsm_begin(candy_gc_t *self) {
  int res = candy_gc_event_handler(self)(self->main, self, EVT_COLOURING);
  assert(res >= 0);
  return 0;
}

static int _fsm_diffusion(candy_gc_t *self) {
  candy_object_t *obj = self->gray;
  /* remove from 'gray' list */
  int res = candy_gc_event_handler(self)(obj, self, EVT_DIFFUSION);
  assert(res >= 0);
  return 0;
}

static int _fsm_end(candy_gc_t *self) {
  for (candy_object_t **it = &self->pool; *it; ) {
    switch (candy_object_get_mark(*it)) {
      case MARK_WHITE:
        _del_node(self, it);
        break;
      case MARK_DARK:
        candy_object_set_mark(*it, MARK_WHITE);
        it = candy_object_get_next(*it);
        break;
      default:
        assert(0);
    }
  }
  return 0;
}

int candy_gc_init(candy_gc_t *self, candy_handler_t handler, candy_allocator_t alloc, void *arg) {
  self->fsm = GC_FSM_BEGIN;
  self->pool = NULL;
  self->gray = NULL;
  self->main = NULL;
  self->handler = handler;
  self->alloc = alloc;
  self->arg = arg;
  self->used = 0;
  return 0;
}

int candy_gc_deinit(candy_gc_t *self) {
  while (self->pool)
    _del_node(self, &self->pool);
  if (self->main)
    candy_gc_event_handler(self)((candy_object_t *)self->main, self, EVT_DELETE);
  return 0;
}

candy_object_t *candy_gc_add(candy_gc_t *self, candy_types_t type, size_t size) {
  return _add_node(self, &self->pool, type, size);
}

int candy_gc_move(candy_gc_t *self, candy_gc_move_t type) {
  candy_object_t *obj = self->pool;
  self->pool = *candy_object_get_next(obj);
  candy_object_set_next(obj, NULL);
  switch (type) {
    case GC_MV_MAIN:
      self->main = obj;
      break;
    default:
      return -1;
  }
  return 0;
}

int candy_gc_sweep(candy_gc_t *self) {
  // for (candy_object_t *obj = self->root, *next = candy_object_get_next(obj); obj;) {
  //   if (candy_object_get_mark(obj) == MARK_DARK)

  // }
  return 0;
}

int candy_gc_step(candy_gc_t *self) {
  switch (self->fsm) {
    case GC_FSM_BEGIN:
      _fsm_begin(self);
      self->fsm = GC_FSM_DIFFUSION;
      return 0;
    case GC_FSM_DIFFUSION:
      if (self->gray)
        _fsm_diffusion(self);
      else
        self->fsm = GC_FSM_END;
      return 0;
    case GC_FSM_END:
      _fsm_end(self);
      self->fsm = GC_FSM_BEGIN;
      return 0;
    default:
      return -1;
  }
}

int candy_gc_full(candy_gc_t *self) {
  if (candy_gc_fsm(self) == GC_FSM_BEGIN)
    candy_gc_step(self);
  while (candy_gc_fsm(self) != GC_FSM_BEGIN)
    candy_gc_step(self);
  return 0;
}
