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
#include "core/candy_closure.h"
#include <string.h>
#include <stdlib.h>

#define vm_assert(_condition, _format, ...) \
candy_assert(&self->ctx, self->gc, _condition, CANDY_ERR_VM, _format, ##__VA_ARGS__)

typedef struct vmll vmll_t;

struct vmll {
  candy_callinfo_t ci;
  candy_vm_t *vm;
  candy_state_t *co;
};

static const char TAG[] = "vm";

static candy_err_t _callinfo_init(candy_callinfo_t *self, candy_callinfo_t *prev, int narg) {
  candy_err_t err = CANDY_OK;
  self->prev = prev;
  self->next = NULL;
  prev->next = self;
  self->bos = prev->tos - narg - 1;
  self->tos = prev->tos;
  self->pc = NULL;
  candy_logi(TAG, "new callinfo: bos %" PRIuPTR ", tos %" PRIuPTR, self->bos, self->tos);
  return err;
}

static candy_err_t _callinfo_deinit(candy_callinfo_t *self, candy_callinfo_t *prev, int nres) {
  candy_err_t err = CANDY_OK;
  prev->next = self->next;
  prev->tos = self->bos + nres;
  candy_logi(TAG, "del callinfo: bos %" PRIuPTR ", tos %" PRIuPTR, prev->bos, prev->tos);
  return err;
}

static inline void _op_loadg(candy_vm_t *self, const candy_inst_t *pc, candy_sclosure_t *clos) {
  candy_logd(TAG, "into %s %s:%d", __FUNCTION__, __FILE__, __LINE__);
  candy_table_t *g = candy_gc_global(self->gc);
  const candy_proto_t *proto = candy_sclosure_get_proto(clos);
  const candy_vector_t *cst = candy_proto_get_const(proto);
  const candy_wrap_t *key = (candy_wrap_t *)candy_vector_data(cst) + pc->iax.a;
  const candy_wrap_t *val = candy_table_get(g, self->gc, key);
  candy_vm_push(self, val, 1);
}

static inline void _op_loadc(candy_vm_t *self, const candy_inst_t *pc, candy_sclosure_t *clos) {
  candy_logd(TAG, "into %s %s:%d", __FUNCTION__, __FILE__, __LINE__);
  const candy_proto_t *proto = candy_sclosure_get_proto(clos);
  const candy_vector_t *cst = candy_proto_get_const(proto);
  const candy_wrap_t *val = (candy_wrap_t *)candy_vector_data(cst) + pc->iax.a;
  candy_vm_push(self, val, 1);
}

static inline void _op_call(candy_vm_t *self, const candy_inst_t *pc, candy_state_t *co) {
  candy_logd(TAG, "into %s %s:%d", __FUNCTION__, __FILE__, __LINE__);
  candy_object_t *out = NULL;
  candy_err_t err = candy_vm_call(self, pc->iabx.a, pc->iabx.b, co, &out);
  (void)err;
}

static void _execute_cfunction(candy_vm_t *self, const candy_wrap_t *fn, candy_state_t *co) {
  candy_logd(TAG, "into %s %s:%d", __FUNCTION__, __FILE__, __LINE__);
  candy_callinfo_t *ci = self->ci;
  candy_cfunc_t cfunc = candy_wrap_get_cfunc(fn);
  ++ci->bos;
  int res = cfunc(co);
  --ci->bos;
  candy_wrap_t *stack = (candy_wrap_t *)candy_vector_data(&self->s);
  for (int i = 0; i < res; ++i) {
    candy_logd(TAG, "mv %" PRIdPTR " to %" PRIdPTR, ci->tos + i - res, ci->bos + i);
    stack[ci->bos + i] = stack[ci->tos + i - res];
    stack[ci->tos + i - res] = CANDY_WRAP_NULL;
  }
}

static void _execute_sclosure(candy_vm_t *self, const candy_wrap_t *fn, candy_state_t *co) {
  candy_logd(TAG, "into %s %s:%d", __FUNCTION__, __FILE__, __LINE__);
  candy_callinfo_t *ci = self->ci;
  candy_sclosure_t *clos = (candy_sclosure_t *)candy_wrap_get_object(fn);
  const candy_vector_t *inst = candy_proto_get_inst(candy_sclosure_get_proto(clos));
  const candy_inst_t *head = candy_vector_data(inst);
  const candy_inst_t *tail = head + candy_vector_size(inst);
  for (const candy_inst_t *it = head; it < tail; ++it) {
    switch ((candy_opcodes_t)it->op) {
      #define CANDY_OPCODE_CASE
      #include "core/candy_opcode.list"
    }
  }
  --ci->bos;
}

static candy_err_t _vmll_init(vmll_t *self, candy_vm_t *vm, int narg, candy_state_t *co) {
  candy_err_t err = CANDY_OK;
  _callinfo_init(&self->ci, vm->ci, narg);
  vm->ci = &self->ci;
  self->vm = vm;
  self->co = co;
  return err;
}

static candy_err_t _vmll_deinit(vmll_t *self, int nres) {
  candy_err_t err = CANDY_OK;
  candy_vm_t *vm = self->vm;
  _callinfo_deinit(&self->ci, vm->ci->prev, nres);
  vm->ci = vm->ci->prev;
  return err;
}

static void _call(vmll_t *self) {
  candy_logi(TAG, "call function at stack index %" PRIdPTR, self->ci.bos);
  const candy_wrap_t *fn = candy_vm_view(self->vm, 0);
  candy_logi(TAG, "call type %s", candy_type_str(candy_wrap_type(fn)));
  switch (candy_wrap_type(fn)) {
    case CANDY_TYPE_CFUNC:
      _execute_cfunction(self->vm, fn, self->co);
      break;
    case CANDY_TYPE_SCLOS:
      _execute_sclosure(self->vm, fn, self->co);
      break;
    default:
      assert(0);
      break;
  }
}

candy_err_t candy_vm_init(candy_vm_t *self, candy_gc_t *gc, candy_excep_t *ctx) {
  candy_err_t err = CANDY_OK;
  candy_excep_init(&self->ctx);
  candy_vector_init(&self->s);
  candy_vector_resize(&self->s, candy_gc_memory(gc), ctx, CANDY_CONFIG_VM_STACK_SIZE, sizeof(candy_wrap_t));
  memset(candy_vector_data(&self->s), 0, candy_vector_size(&self->s) * sizeof(candy_wrap_t));
  memset(&self->base_ci, 0, sizeof(candy_callinfo_t));
  self->ci = &self->base_ci;
  self->gc = gc;
  return err;
}

candy_err_t candy_vm_deinit(candy_vm_t *self) {
  candy_err_t err = CANDY_OK;
  self->ci = NULL;
  candy_vector_deinit(&self->s, candy_gc_memory(self->gc), sizeof(candy_wrap_t));
  candy_excep_deinit(&self->ctx);
  self->gc = NULL;
  return err;
}

candy_err_t candy_vm_call(candy_vm_t *self, int narg, int nres, candy_state_t *co, candy_object_t **out) {
  vmll_t vmll;
  _vmll_init(&vmll, self, narg, co);
  candy_err_t err = candy_excep_try(&self->ctx, (candy_excep_cb_t)_call, &vmll, out);
  _vmll_deinit(&vmll, nres);
  return err;
}

candy_err_t candy_vm_pop(candy_vm_t *self, size_t n) {
  candy_err_t err = CANDY_OK;
  ptrdiff_t tos = self->ci->tos;
  ptrdiff_t bos = self->ci->bos;
  vm_assert(tos >= bos + (ptrdiff_t)n, "stack index out of bounds");
  self->ci->tos -= n;
  candy_logi(TAG, "pop %" PRIdPTR " items from stack, new tos %" PRIdPTR, n, self->ci->tos);
  return err;
}

candy_err_t candy_vm_push(candy_vm_t *self, const candy_wrap_t *wrap, size_t n) {
  candy_err_t err = CANDY_OK;
  size_t size = candy_vector_size(&self->s);
  ptrdiff_t tos = self->ci->tos;
  vm_assert(tos + n <= size, "stack overflow");
  memcpy((candy_wrap_t *)candy_vector_data(&self->s) + tos, wrap, sizeof(candy_wrap_t) * n);
  self->ci->tos += n;
  candy_logi(TAG, "push %" PRIdPTR " items to stack, new tos %" PRIdPTR, n, self->ci->tos);
  return err;
}

const candy_wrap_t *candy_vm_view(candy_vm_t *self, int idx) {
  ptrdiff_t bos = self->ci->bos;
  ptrdiff_t tos = self->ci->tos;
  ptrdiff_t offset = (idx < 0 ? tos : bos) + idx;
  candy_logi(TAG, "view stack index %d (real %" PRIdPTR ") bos %" PRIdPTR " tos %" PRIdPTR, idx, offset, bos, tos);
  vm_assert(offset >= bos, "stack index out of bounds");
  vm_assert(offset < tos, "stack index out of bounds");
  return (const candy_wrap_t *)candy_vector_data(&self->s) + offset;
}
