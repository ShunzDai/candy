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
#ifndef CANDY_CORE_GC_H
#define CANDY_CORE_GC_H
#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#include "core/candy_priv.h"

typedef enum candy_events {
  EVT_DELETE,
  EVT_COLOURING,
  EVT_DIFFUSION,
} candy_events_t;

typedef enum cnady_gc_move {
  GC_MV_MAIN_STATE,
} candy_gc_move_t;

typedef int (*candy_handler_t)(candy_object_t *self, candy_gc_t *gc, candy_events_t evt);

struct candy_gc {
  candy_object_t *pool;
  candy_state_t *main;
  candy_handler_t handler;
  candy_allocator_t alloc;
  void *arg;
};

int candy_gc_init(candy_gc_t *self, candy_handler_t handler, candy_allocator_t alloc, void *arg);
int candy_gc_deinit(candy_gc_t *self);

candy_object_t *candy_gc_add(candy_gc_t *self, candy_types_t type, size_t size);

int candy_gc_move(candy_gc_t *self, candy_gc_move_t type);

static inline candy_state_t *candy_gc_main(candy_gc_t *self) {
  return self->main;
}

static inline void *candy_gc_alloc(candy_gc_t *self, void *ptr, size_t old_size, size_t new_size) {
  return self->alloc(ptr, old_size, new_size, self->arg);
}

#ifdef __cplusplus
}
#endif /* __cplusplus */
#endif /* CANDY_CORE_GC_H */
