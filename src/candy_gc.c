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
#include "src/candy_gc.h"
#include "src/candy_object.h"
#include <assert.h>

static candy_object_t *candy_gc_add_node(candy_gc_t *self, candy_object_t **pos, candy_types_t type, size_t size) {
  candy_object_t *obj = (candy_object_t *)self->alloc(NULL, 0, size, self->arg);
  candy_object_set_type(obj, type);
  candy_object_set_next(obj, *pos);
  *pos = obj;
  return obj;
}

static void candy_gc_del_node(candy_gc_t *self, candy_object_t **pos, candy_handler_t list[]) {
  candy_object_t *obj = *pos;
  *pos = candy_object_get_next(obj);
  assert(list[candy_object_get_type(obj)]);
  list[candy_object_get_type(obj)](obj, self);
}

int candy_gc_init(candy_gc_t *self, candy_allocator_t alloc, void *arg) {
  self->pool = NULL;
  self->root = NULL;
  self->alloc = alloc;
  self->arg = arg;
  return 0;
}

int candy_gc_deinit(candy_gc_t *self, candy_handler_t list[]) {
  while (self->pool)
    candy_gc_del_node(self, &self->pool, list);
  while (self->root)
    candy_gc_del_node(self, &self->root, list);
  return 0;
}

candy_object_t *candy_gc_add(candy_gc_t *self, candy_types_t type, size_t size) {
  return candy_gc_add_node(self, &self->root, type, size);
}

int candy_gc_sweep(candy_gc_t *self) {

  return 0;
}

int candy_gc_run(candy_gc_t *self) {
  // candy_gc_mark(self);
  candy_gc_sweep(self);
  return 0;
}