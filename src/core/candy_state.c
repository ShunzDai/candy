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
#include "core/candy_state.h"
#include "core/candy_object.h"
#include "core/candy_wrap.h"
#include "core/candy_gc.h"
#include "core/candy_table.h"
#include "core/candy_array.h"
#include "core/candy_parser.h"
#include "core/candy_vm.h"
#include <string.h>

typedef struct candy_primary candy_primary_t;

struct candy_state {
  candy_object_t header;
  candy_vm_t vm;
  candy_object_t *gray;
};

struct candy_primary {
  candy_state_t co;
  candy_gc_t gc;
};

struct pack_primary {
  candy_excep_t ctx;
  candy_handler_t handler;
  candy_allocator_t alloc;
  void *arg;
  candy_state_t *co;
};

struct pack_call {
  candy_state_t *co;
  candy_reader_t reader;
  void *arg;
};

static size_t candy_state_size(candy_state_t *self) {
  return candy_state_is_primary(self) ? sizeof(candy_primary_t) : sizeof(candy_state_t);
}

static candy_err_t candy_state_init(candy_state_t *self, candy_gc_t *gc, candy_excep_t *ctx) {
  candy_vm_init(&self->vm, gc, ctx);
  self->gray = NULL;
  return CANDY_OK;
}

static candy_err_t candy_state_deinit(candy_state_t *self) {
  candy_vm_deinit(&self->vm);
  return CANDY_OK;
}

static void _create(void *arg) {
  struct pack_primary *self = (struct pack_primary *)arg;
  candy_gc_t gc;
  candy_gc_init(&gc, &self->ctx, self->handler, self->alloc, self->arg);
  candy_primary_t *p = (candy_primary_t *)candy_gc_add_primary(&gc, &self->ctx, sizeof(candy_primary_t));
  memcpy(&p->gc, &gc, sizeof(candy_gc_t));
  candy_state_init(&p->co, &p->gc, &self->ctx);
  candy_table_create_global(&p->gc, &self->ctx);
  self->co = &p->co;
}

static void _dostream(void *arg) {
  struct pack_call *self = (struct pack_call *)arg;
  candy_vm_t *vm = &self->co->vm;
  candy_object_t *out = NULL;
  candy_err_t err = candy_parse(vm->gc, &vm->ctx, self->reader, self->arg, &out);
  if (err != CANDY_OK || out == NULL) {
    candy_excep_throw(&vm->ctx, err, out);
    /* This code is unreachable */
    assert(0);
  }
  err = candy_state_push_object(self->co, out);
  err = candy_vm_call(vm, 0, 1, self->co, &out);
}

static candy_err_t _state_delete(candy_state_t *self, candy_gc_t *gc, void *arg) {
  size_t size = candy_state_size(self);
  candy_state_deinit(self);
  candy_gc_free(gc, self, size);
  return CANDY_OK;
}

static candy_err_t _state_color(candy_state_t *self, candy_gc_t *gc, void *arg) {
  self->gray = candy_gc_gray_swap(gc, (candy_object_t *)self);
  candy_object_set_mark((candy_object_t *)self, MARK_GRAY);
  return CANDY_OK;
}

static candy_err_t _state_diffuse(candy_state_t *self, candy_gc_t *gc, void *arg) {
  candy_gc_gray_swap(gc, self->gray);
  candy_object_set_mark((candy_object_t *)self, MARK_DARK);
  return CANDY_OK;
}

candy_state_t *candy_state_create(candy_handler_t handler, candy_allocator_t alloc, void *arg) {
  struct pack_primary pack = {
    .handler = handler,
    .alloc = alloc,
    .arg = arg,
    .co = NULL,
  };
  candy_excep_init(&pack.ctx);
  candy_err_t err = candy_excep_try(&pack.ctx, _create, &pack, NULL);
  candy_excep_deinit(&pack.ctx);
  if (err != CANDY_OK)
    return NULL;
  return pack.co;
}

candy_state_t *candy_state_create_coroutine(candy_state_t *self) {
  // candy_state_t *co = (candy_state_t *)candy_gc_add(self->vm.gc), candy_vm_excep(&self->vm), CANDY_TYPE_STATE, sizeof(candy_state_t));
  // candy_state_init(co, self->vm.gc);
  // return co;
  return NULL;
}

candy_err_t candy_state_handler(candy_state_t *self, candy_gc_t *gc, candy_events_t evt, void *arg) {
  switch (evt) {
    case EVT_DELETE:  return _state_delete(self, gc, arg);
    case EVT_COLOR:   return _state_color(self, gc, arg);
    case EVT_DIFFUSE: return _state_diffuse(self, gc, arg);
    default:          return CANDY_ERR;
  }
}

candy_err_t candy_state_close(candy_state_t *self) {
  candy_err_t err = CANDY_OK;
  if (candy_state_is_primary(self)) {
    candy_gc_t gc;
    memcpy(&gc, self->vm.gc, sizeof(candy_gc_t));
    self->vm.gc = &gc;
    err = candy_gc_deinit(&gc);
  }
  return err;
}

candy_err_t candy_state_dostream(candy_state_t *self, candy_reader_t reader, void *arg) {
  struct pack_call pack = {
    .co = self,
    .reader = reader,
    .arg = arg,
  };
  candy_array_t *out = NULL;
  candy_err_t err = candy_excep_try(&self->vm.ctx, _dostream, &pack, (candy_object_t **)&out);
  if (err != CANDY_OK && out != NULL) {
    printf("%.*s\n", (int)candy_array_size(out), (char *)candy_array_data(out));
  }
  return err;
}

candy_err_t candy_state_call(candy_state_t *self, int narg, int nres) {
  candy_array_t *out = NULL;
  candy_err_t err = candy_vm_call(&self->vm, narg, nres, self, (candy_object_t **)&out);
  return err;
}

candy_err_t candy_state_setglobal(candy_state_t *self, const char name[], candy_cfunc_t entry) {
  candy_err_t err = CANDY_OK;
  candy_wrap_t key, val;
  candy_table_t *g = candy_gc_global(self->vm.gc);
  const candy_array_t *s = candy_array_create_const(self->vm.gc, &self->vm.ctx, CANDY_TYPE_CHAR, name, strlen(name));
  candy_wrap_set_object(&key, (candy_object_t *)s);
  candy_wrap_set_cfunc(&val, entry);
  err = candy_table_set(g, self->vm.gc, &self->vm.ctx, &key, &val);
  return err;
}

candy_err_t candy_state_push_integer(candy_state_t *self, candy_integer_t val) {
  candy_wrap_t wrap;
  candy_wrap_set_integer(&wrap, val);
  candy_err_t err = candy_vm_push(&self->vm, &wrap, 1);
  return err;
}

candy_err_t candy_state_push_float(candy_state_t *self, candy_float_t val) {
  candy_wrap_t wrap;
  candy_wrap_set_float(&wrap, val);
  candy_err_t err = candy_vm_push(&self->vm, &wrap, 1);
  return err;
}

candy_err_t candy_state_push_object(candy_state_t *self, const candy_object_t *val) {
  candy_wrap_t wrap;
  candy_wrap_set_object(&wrap, val);
  candy_err_t err = candy_vm_push(&self->vm, &wrap, 1);
  return err;
}

candy_err_t candy_state_push_cfunc(candy_state_t *self, candy_cfunc_t val) {
  candy_wrap_t wrap;
  candy_wrap_set_cfunc(&wrap, val);
  candy_err_t err = candy_vm_push(&self->vm, &wrap, 1);
  return err;
}

candy_integer_t candy_state_to_integer(candy_state_t *self, int idx) {
  const candy_wrap_t *wrap = candy_vm_view(&self->vm, idx);
  candy_integer_t val = candy_wrap_get_integer(wrap);
  return val;
}

candy_float_t candy_state_to_float(candy_state_t *self, int idx) {
  const candy_wrap_t *wrap = candy_vm_view(&self->vm, idx);
  candy_float_t val = candy_wrap_get_float(wrap);
  return val;
}

candy_object_t *candy_state_to_object(candy_state_t *self, int idx) {
  const candy_wrap_t *wrap = candy_vm_view(&self->vm, idx);
candy_object_t *val = candy_wrap_get_object(wrap);
  return val;
}

candy_cfunc_t candy_state_to_cfunc(candy_state_t *self, int idx) {
  const candy_wrap_t *wrap = candy_vm_view(&self->vm, idx);
  candy_cfunc_t val = candy_wrap_get_cfunc(wrap);
  return val;
}

bool candy_state_is_primary(candy_state_t *self) {
  return candy_gc_primary(self->vm.gc) == self;
}
