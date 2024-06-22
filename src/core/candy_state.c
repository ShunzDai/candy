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
  candy_exce_t ctx;
  candy_vm_t vm;
  candy_gc_t *gc;
  candy_object_t *gray;
};

struct candy_primary {
  candy_state_t co;
  candy_gc_t gc;
  bool is_deleting;
};

struct protect_create_arg {
  candy_state_t *co;
  candy_gc_t *gc;
  candy_exce_t *ctx;
};

static size_t candy_state_size(candy_state_t *self) {
  return candy_state_is_main(self) ? sizeof(struct candy_primary) : sizeof(struct candy_state);
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

static void protect_create(struct protect_create_arg *arg) {
  candy_primary_t *p = (candy_primary_t *)candy_gc_add(arg->gc, arg->ctx, CANDY_BASE_STATE, sizeof(struct candy_primary));
  memcpy(&p->gc, arg->gc, sizeof(struct candy_gc));
  candy_gc_move(&p->gc, GC_MV_MAIN);
  arg->co = &p->co;
  candy_state_init(arg->co, &p->gc);
}

candy_state_t *candy_state_create(candy_gc_t *gc) {
  candy_exce_t ctx;
  struct protect_create_arg arg = {
    .co = NULL,
    .gc = gc,
    .ctx = &ctx,
  };
  candy_exce_init(&ctx);
  candy_err_t err = candy_exce_try(&ctx, (candy_exce_cb_t)protect_create, &arg, NULL);
  candy_exce_deinit(&ctx);
  if (err != EXCE_OK)
    return NULL;
  return arg.co;
}

candy_state_t *candy_state_create_coroutine(candy_state_t *self) {
  candy_state_t *co = (candy_state_t *)candy_gc_add(self->gc, &self->ctx, CANDY_BASE_STATE, sizeof(struct candy_state));
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
  memcpy(&gc, self->gc, sizeof(struct candy_gc));
  candy_gc_deinit(&gc);
  return 0;
}

int candy_state_colouring(candy_state_t *self, candy_gc_t *gc) {
  self->gray = candy_gc_gray_swap(gc, (candy_object_t *)self);
  candy_object_set_mark((candy_object_t *)self, MARK_GRAY);
  return 0;
}

int candy_state_diffusion(candy_state_t *self, candy_gc_t *gc) {
  candy_gc_gray_swap(gc, self->gray);
  candy_object_set_mark((candy_object_t *)self, MARK_DARK);
  return 0;
}

int candy_state_dostream(candy_state_t *self, candy_reader_t reader, void *arg) {
  candy_exce_init(&self->ctx);
  candy_object_t *out = candy_parse(self->gc, &self->ctx, reader, arg);
  if (candy_object_get_type(out) == CANDY_BASE_CHAR)
    printf("%.*s\n",
      (int)candy_array_size((candy_array_t *)out),
      (char *)candy_array_data((candy_array_t *)out)
    );
  // candy_vm_execute(&self->vm, out);
  candy_gc_full(self->gc);
  candy_exce_deinit(&self->ctx);
  return 0;
}

bool candy_state_is_main(candy_state_t *self) {
  return candy_gc_main(self->gc) == (candy_object_t *)self;
}
