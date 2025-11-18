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
#include "core/candy_vm.h"
#include "core/candy_gc.h"
#include "core/candy_wrap.h"
#include "core/candy_array.h"
#include "core/candy_table.h"
#include "core/candy_proto.h"
#include <string.h>
#include <stdlib.h>

#define vm_assert(_condition, _format, ...) \
candy_assert(&self->ctx, self->gc, _condition, CANDY_ERR_VM, _format, ##__VA_ARGS__)

typedef struct vmll vmll_t;

struct candy_callinfo {
  candy_callinfo_t *prev;
  candy_callinfo_t *next;
  ptrdiff_t tos;
  ptrdiff_t bos;
  candy_inst_t *pc;
  int nres;
};

struct vmll {
  candy_vm_t *vm;
  candy_state_t *co;
};

static candy_err_t _vmll_init(vmll_t *self, candy_vm_t *vm, int narg, int nres, candy_state_t *co) {
  self->vm = vm;
  self->co = co;
  return CANDY_OK;
}

static candy_err_t _vmll_deinit(vmll_t *self) {
  return CANDY_OK;
}

// static inline void _op_move(candy_vm_t *self, const candy_inst_t *pc) {
//   candy_wrap_t *reg = NULL;
//   reg[pc->iabx.a] = reg[pc->iabx.b];
// }

// static inline void _op_loadc(candy_vm_t *self, const candy_inst_t *pc) {
//   candy_wrap_t *reg = NULL;
//   reg[pc->iabx.a] = reg[pc->iabx.b];
// }

static candy_callinfo_t *_vmll_call(vmll_t *self, const candy_wrap_t *fn, const candy_callinfo_t *ci) {
  candy_cfunc_t f = candy_wrap_get_cfunc(fn);
  int res = f(self->co);
  (void)res;
  return NULL;
}

static void _execute(vmll_t *self, candy_callinfo_t *ci) {
  const candy_inst_t *head = NULL;
  const candy_inst_t *tail = head;
  for (const candy_inst_t *it = head; it < tail; ++it) {
    switch ((candy_opcodes_t)it->op) {
      #define CANDY_OPCODE_CASE
      #include "core/candy_opcode.list"
    }
  }
}

static void _call(vmll_t *self) {
  candy_callinfo_t *ci = self->vm->ci;
  const candy_wrap_t *fn = candy_vm_view(self->vm, ci->bos);
  switch (candy_wrap_type(fn)) {
    case CANDY_TYPE_CFUNC:
      _vmll_call(self, fn, ci);
      break;
    case CANDY_TYPE_SCLOS:
      _execute(self, self->vm->ci);
      break;
    default:
      // vm_assert(0, "unknown function type");
      break;
  }
}

static candy_callinfo_t *_new_callinfo(candy_callinfo_t *self, candy_gc_t *gc, candy_excep_t *ctx) {
  candy_callinfo_t *ci = candy_gc_alloc(gc, ctx, sizeof(candy_callinfo_t));
  ci->prev = self;
  ci->next = NULL;
  ci->bos = 0;
  ci->tos = 0;
  return ci;
}

static candy_err_t _del_callinfo(candy_callinfo_t *self, candy_gc_t *gc) {
  candy_err_t err = CANDY_OK;
  candy_gc_free(gc, self, sizeof(candy_callinfo_t));
  return err;
}

candy_err_t candy_vm_init(candy_vm_t *self, candy_gc_t *gc) {
  candy_excep_init(&self->ctx);
  candy_vector_init(&self->s);
  candy_vector_resize(&self->s, candy_gc_memory(gc), &self->ctx, CANDY_CONFIG_VM_STACK_SIZE, sizeof(candy_wrap_t));
  self->ci = _new_callinfo(self->ci, gc, &self->ctx);
  self->gc = gc;
  return CANDY_OK;
}

candy_err_t candy_vm_deinit(candy_vm_t *self) {
  _del_callinfo(self->ci, self->gc);
  self->ci = NULL;
  candy_vector_deinit(&self->s, candy_gc_memory(self->gc), sizeof(candy_wrap_t));
  candy_excep_deinit(&self->ctx);
  self->gc = NULL;
  return CANDY_OK;
}

candy_err_t candy_vm_call(candy_vm_t *self, int narg, int nres, candy_state_t *co, candy_object_t **out) {
  vmll_t vmll;
  _vmll_init(&vmll, self, narg, nres, co);
  candy_err_t err = candy_excep_try(&self->ctx, (candy_excep_cb_t)_call, &vmll, out);
  _vmll_deinit(&vmll);
  return err;
}

candy_err_t candy_vm_pop(candy_vm_t *self, size_t n) {
  vm_assert(self->ci->tos >= self->ci->bos + (ptrdiff_t)n, "stack index out of bounds");
  self->ci->tos -= n;
  return CANDY_OK;
}

candy_err_t candy_vm_push(candy_vm_t *self, const candy_wrap_t *wrap, size_t n) {
  size_t size = candy_vector_size(&self->s);
  vm_assert(self->ci->tos + n <= size, "stack overflow");
  memcpy((candy_wrap_t *)candy_vector_data(&self->s) + self->ci->tos, wrap, sizeof(candy_wrap_t) * n);
  self->ci->tos += n;
  return CANDY_OK;
}

const candy_wrap_t *candy_vm_view(candy_vm_t *self, int idx) {
  ptrdiff_t offset = (idx < 0 ? self->ci->tos : self->ci->bos) + idx;
  vm_assert(offset >= self->ci->bos, "stack index out of bounds");
  vm_assert(offset < self->ci->tos, "stack index out of bounds");
  return (const candy_wrap_t *)candy_vector_data(&self->s) + offset;
}
