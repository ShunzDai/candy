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

static candy_object_t *candy_gc_add_node(candy_gc_t *self, candy_object_t **pos, candy_types_t type, size_t size) {
  candy_object_t *obj = (candy_object_t *)candy_gc_alloc(self, NULL, 0, size);
  candy_object_set_next(obj, *pos);
  candy_object_set_type(obj, type);
  candy_object_set_mask(obj, MASK_NONE);
  candy_object_set_mark(obj, MARK_WHITE);
  *pos = obj;
  return obj;
}

static void candy_gc_del_node(candy_gc_t *self, candy_object_t **pos) {
  candy_object_t *obj = *pos;
  *pos = candy_object_get_next(obj);
  self->handler(obj, self, EVT_DELETE);
}

int candy_gc_init(candy_gc_t *self, candy_handler_t handler, candy_allocator_t alloc, void *arg) {
  self->pool = NULL;
  self->handler = handler;
  self->alloc = alloc;
  self->arg = arg;
  return 0;
}

int candy_gc_deinit(candy_gc_t *self) {
  while (self->pool)
    candy_gc_del_node(self, &self->pool);
  if (self->main)
    self->handler((candy_object_t *)self->main, self, EVT_DELETE);
  return 0;
}

candy_object_t *candy_gc_add(candy_gc_t *self, candy_types_t type, size_t size) {
  return candy_gc_add_node(self, &self->pool, type, size);
}

int candy_gc_move(candy_gc_t *self, candy_gc_move_t type) {
  candy_object_t *obj = self->pool;
  self->pool = candy_object_get_next(obj);
  switch (type) {
    case GC_MV_MAIN_STATE:
      self->main = (candy_state_t *)obj;
      break;
    default:
      return -1;
  }
  return 0;
}

int candy_gc_sweep(candy_gc_t *self) {

  return 0;
}

int candy_gc_run(candy_gc_t *self) {
  // candy_gc_mark(self);
  candy_gc_sweep(self);
  return 0;
}
