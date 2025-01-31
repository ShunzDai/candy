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
#include "core/candy_state.h"
#include "core/candy_object.h"
#include "core/candy_exception.h"
#include "core/candy_gc.h"
#include "core/candy_array.h"
#include "core/candy_parser.h"
#include "core/candy_vm.h"
#include <string.h>

typedef struct candy_primary candy_primary_t;

struct candy_state {
  candy_object_t header;
  candy_excep_t ctx;
  candy_vm_t vm;
  candy_gc_t *gc;
  candy_object_t *gray;
};

struct candy_primary {
  candy_state_t co;
  candy_gc_t gc;
};

struct pack {
  candy_excep_t ctx;
  candy_gc_t *gc;
  candy_state_t *co;
};

static size_t candy_state_size(candy_state_t *self) {
  return candy_state_is_primary(self) ? sizeof(candy_primary_t) : sizeof(candy_state_t);
}

static int candy_state_init(candy_state_t *self, candy_gc_t *gc) {
  self->gc = gc;
  self->gray = NULL;
  candy_vm_init(&self->vm);
  return 0;
}

static int candy_state_deinit(candy_state_t *self) {
  candy_vm_deinit(&self->vm, self->gc);
  return 0;
}

// static candy_primary_t *get_primary(candy_state_t *self) {
//   return (candy_primary_t *)candy_gc_main(self->gc);
// }

static void protect_create(struct pack *self) {
  candy_primary_t *p = (candy_primary_t *)candy_gc_add(self->gc, &self->ctx, CANDY_TYPE_STATE, sizeof(candy_primary_t));
  memcpy(&p->gc, self->gc, sizeof(candy_gc_t));
  candy_gc_move(&p->gc, GC_MV_PRIM);
  candy_state_init(&p->co, &p->gc);
  self->co = &p->co;
}

candy_state_t *candy_state_create(candy_handler_t handler, candy_allocator_t alloc, void *arg) {
  candy_gc_t gc;
  struct pack pack = {
    .gc = &gc,
    .co = NULL,
  };
  candy_gc_init(&gc, handler, alloc, arg);
  candy_excep_init(&pack.ctx);
  candy_err_t err = candy_excep_try(&pack.ctx, (candy_excep_cb_t)protect_create, &pack, NULL);
  candy_excep_deinit(&pack.ctx);
  if (err != CANDY_OK)
    return NULL;
  return pack.co;
}

candy_state_t *candy_state_create_coroutine(candy_state_t *self) {
  candy_state_t *co = (candy_state_t *)candy_gc_add(self->gc, &self->ctx, CANDY_TYPE_STATE, sizeof(candy_state_t));
  candy_state_init(co, self->gc);
  return co;
}

int candy_state_delete(candy_state_t *self, candy_gc_t *gc) {
  candy_state_deinit(self);
  candy_gc_free(gc, self, candy_state_size(self));
  return 0;
}

int candy_state_close(candy_state_t *self) {
  candy_gc_t gc;
  memcpy(&gc, self->gc, sizeof(candy_gc_t));
  candy_gc_deinit(&gc);
  return 0;
}

int candy_state_color(candy_state_t *self, candy_gc_t *gc) {
  self->gray = candy_gc_gray_swap(gc, (candy_object_t *)self);
  candy_object_set_mark((candy_object_t *)self, MARK_GRAY);
  return 0;
}

int candy_state_diffuse(candy_state_t *self, candy_gc_t *gc) {
  candy_gc_gray_swap(gc, self->gray);
  candy_object_set_mark((candy_object_t *)self, MARK_DARK);
  return 0;
}

candy_err_t candy_state_dostream(candy_state_t *self, candy_reader_t reader, void *arg) {
  candy_object_t *out = NULL;
  candy_err_t err = candy_parse(self->gc, reader, arg, &out);
  if (err == CANDY_OK) {
    // err = candy_vm_execute(&self->vm, self->gc);
  }
  else {
    printf("%.*s\n", (int)candy_array_size((candy_array_t *)out), (char *)candy_array_data((candy_array_t *)out));
  }
  candy_gc_full(self->gc);
  return err;
}

bool candy_state_is_primary(candy_state_t *self) {
  return candy_gc_primary(self->gc) == (candy_object_t *)self;
}
