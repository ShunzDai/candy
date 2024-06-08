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
  GC_MV_MAIN,
} candy_gc_move_t;

typedef enum cnady_gc_fsm {
  GC_FSM_BEGIN,
  GC_FSM_DIFFUSION,
  GC_FSM_END,
} candy_gc_fsm_t;

typedef int (*candy_handler_t)(candy_object_t *self, candy_gc_t *gc, candy_events_t evt);

struct candy_gc {
  candy_gc_fsm_t fsm;
  candy_object_t *pool;
  candy_object_t *gray;
  candy_object_t *main;
  candy_handler_t handler;
  candy_allocator_t alloc;
  void *arg;
  size_t used;
};

int candy_gc_init(candy_gc_t *self, candy_handler_t handler, candy_allocator_t alloc, void *arg);
int candy_gc_deinit(candy_gc_t *self);

candy_object_t *candy_gc_add(candy_gc_t *self, candy_types_t type, size_t size);

int candy_gc_move(candy_gc_t *self, candy_gc_move_t type);

int candy_gc_step(candy_gc_t *self);

int candy_gc_full(candy_gc_t *self);

static inline candy_gc_fsm_t candy_gc_fsm(candy_gc_t *self) {
  return self->fsm;
}

static inline candy_object_t *candy_gc_gray_swap(candy_gc_t *self, candy_object_t *obj) {
  candy_object_t *gray = self->gray;
  self->gray = obj;
  return gray;
}

static inline candy_object_t *candy_gc_main(candy_gc_t *self) {
  return self->main;
}

static inline candy_handler_t candy_gc_event_handler(candy_gc_t *self) {
  return self->handler;
}

static inline void *candy_gc_alloc(candy_gc_t *self, void *ptr, size_t old_size, size_t new_size) {
  self->used += new_size - old_size;
  return self->alloc(ptr, old_size, new_size, self->arg);
}

static inline size_t candy_gc_used(const candy_gc_t *self) {
  return self->used;
}

#ifdef __cplusplus
}
#endif /* __cplusplus */
#endif /* CANDY_CORE_GC_H */
