/**
  * Copyright 2022-2026 ShunzDai
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
candy_assert(ctx, self->gc, _condition, CANDY_ERR_VM, _format, ##__VA_ARGS__)

#if CANDY_CONFIG_VM_USE_JMPTABLE
#define vmdispatch(_opcode) goto *dispatch_table[(uint32_t)(_opcode)]; if (0)
#define vmcase(_opcode)     L_##_opcode
#define vmbreak()           continue
#else
#define vmdispatch(_opcode) switch ((candy_opcodes_t)(_opcode))
#define vmcase(_opcode)     case OP_##_opcode
#define vmbreak()           break
#endif /* CANDY_CONFIG_VM_USE_JMPTABLE */

typedef struct vmll vmll_t;

struct vmll {
  candy_callinfo_t ci;
  candy_excep_t *ctx;
  candy_vm_t *vm;
  candy_state_t *co;
};

static const char TAG[] = "vm";

static void _call(vmll_t *self);
static candy_err_t _vmll_init(vmll_t *self, candy_vm_t *vm, candy_excep_t *ctx, int narg, candy_state_t *co);
static candy_err_t _vmll_deinit(vmll_t *self, int nres);

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

static CANDY_FORCE_INLINE void _vm_pop(candy_vm_t *self, candy_excep_t *ctx, size_t n) {
  candy_callinfo_t *ci = self->ci;
  ptrdiff_t tos = ci->tos;
  ptrdiff_t bos = ci->bos;
  vm_assert(tos >= bos + (ptrdiff_t)n, "stack overflow");
  ci->tos -= n;
}

static CANDY_FORCE_INLINE void _vm_push(candy_vm_t *self, candy_excep_t *ctx, const candy_wrap_t *wrap, ptrdiff_t n) {
  candy_callinfo_t *ci = self->ci;
  ptrdiff_t cap = (ptrdiff_t)candy_vector_size(&self->s);
  vm_assert(self->ci->tos + n <= cap, "stack overflow");
  candy_wrap_t *stack = (candy_wrap_t *)candy_vector_data(&self->s);
  memcpy(stack + ci->tos, wrap, sizeof(candy_wrap_t) * n);
  ci->tos += n;
}

static CANDY_FORCE_INLINE const candy_wrap_t *_vm_view(candy_vm_t *self, candy_excep_t *ctx, int idx) {
  candy_callinfo_t *ci = self->ci;
  ptrdiff_t bos = ci->bos;
  ptrdiff_t tos = ci->tos;
  ptrdiff_t offset = (idx < 0 ? tos : bos) + idx;
  vm_assert(offset >= bos && offset < tos, "stack overflow");
  return (const candy_wrap_t *)candy_vector_data(&self->s) + offset;
}

static CANDY_FORCE_INLINE void _op_loadg(vmll_t *self, const candy_inst_t *pc, const candy_vector_t *cst) {
  candy_logd(TAG, "into %s %s:%d", __FUNCTION__, __FILE__, __LINE__);
  const candy_wrap_t *key = (const candy_wrap_t *)candy_vector_data(cst) + pc->iax.a;
  const candy_wrap_t *val = candy_table_get(candy_gc_global(self->vm->gc), self->vm->gc, key);
  _vm_push(self->vm, self->ctx, val, 1);
}

static CANDY_FORCE_INLINE void _op_loadc(vmll_t *self, const candy_inst_t *pc, const candy_vector_t *cst) {
  candy_logd(TAG, "into %s %s:%d", __FUNCTION__, __FILE__, __LINE__);
  const candy_wrap_t *val = (const candy_wrap_t *)candy_vector_data(cst) + pc->iax.a;
  _vm_push(self->vm, self->ctx, val, 1);
}

static CANDY_FORCE_INLINE void _op_call(vmll_t *self, const candy_inst_t *pc, candy_state_t *co) {
  candy_logd(TAG, "into %s %s:%d", __FUNCTION__, __FILE__, __LINE__);
  vmll_t vmll;
  _vmll_init(&vmll, self->vm, self->ctx, (int)pc->iabx.a, co);
  _call(&vmll);
  _vmll_deinit(&vmll, (int)pc->iabx.b);
}

static void _execute_cfunction(vmll_t *self, const candy_wrap_t *fn, candy_state_t *co) {
  candy_logd(TAG, "into %s %s:%d", __FUNCTION__, __FILE__, __LINE__);
  candy_callinfo_t *ci = &self->ci;
  candy_cfunc_t cfunc = candy_wrap_get_cfunc(fn);
  ++ci->bos;
  int res = cfunc(co);
  --ci->bos;
  candy_wrap_t *stack = (candy_wrap_t *)candy_vector_data(&self->vm->s);
  for (int i = 0; i < res; ++i) {
    candy_logd(TAG, "mv %" PRIdPTR " to %" PRIdPTR, ci->tos + i - res, ci->bos + i);
    stack[ci->bos + i] = stack[ci->tos + i - res];
    stack[ci->tos + i - res] = CANDY_WRAP_NULL;
  }
}

static void _execute_sclosure(vmll_t *self, const candy_wrap_t *fn, candy_state_t *co) {
  candy_logd(TAG, "into %s %s:%d", __FUNCTION__, __FILE__, __LINE__);
  candy_sclosure_t *clos = (candy_sclosure_t *)candy_wrap_get_object(fn);
  const candy_proto_t *proto = candy_sclosure_get_proto(clos);
  const candy_vector_t *inst = candy_proto_get_inst(proto);
  const candy_vector_t *cst = candy_proto_get_const(proto);

  #if CANDY_CONFIG_VM_USE_JMPTABLE
  #if defined(__GNUC__)
  static void *dispatch_table[] = {
    #define CANDY_OPCODE_LABEL
    #include "core/candy_opcode.list"
  };
  #else
  #error "VM jump table dispatch requires GNU C compiler"
  #endif
  #endif

  for (size_t pc = 0; pc < candy_vector_size(inst); ) {
    const candy_inst_t *it = (const candy_inst_t *)candy_vector_data(inst) + pc;
    ++pc;
    vmdispatch(it->op) {
      #define CANDY_OPCODE_CASE
      #include "core/candy_opcode.list"
    }
  }
}

static candy_err_t _vmll_init(vmll_t *self, candy_vm_t *vm, candy_excep_t *ctx, int narg, candy_state_t *co) {
  candy_err_t err = CANDY_OK;
  _callinfo_init(&self->ci, vm->ci, narg);
  vm->ci = &self->ci;
  self->ctx = ctx;
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
  candy_logi(TAG, "call function at stack %" PRIdPTR, self->ci.bos);
  const candy_wrap_t *fn = _vm_view(self->vm, self->ctx, 0);
  candy_logi(TAG, "call type %s", candy_type_str(candy_wrap_type(fn)));
  switch (candy_wrap_type(fn)) {
    case CANDY_TYPE_CFUNC:
      _execute_cfunction(self, fn, self->co);
      break;
    case CANDY_TYPE_SCLOS:
      _execute_sclosure(self, fn, self->co);
      break;
    default:
      assert(0);
      break;
  }
}

candy_err_t candy_vm_init(candy_vm_t *self, candy_gc_t *gc, candy_excep_t *ctx) {
  candy_err_t err = CANDY_OK;
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
  self->gc = NULL;
  return err;
}

candy_err_t candy_vm_call(candy_vm_t *self, candy_excep_t *ctx, int narg, int nres, candy_state_t *co, candy_object_t **out) {
  vmll_t vmll;
  _vmll_init(&vmll, self, ctx, narg, co);
  candy_err_t err = candy_excep_try(ctx, (candy_excep_cb_t)_call, &vmll, out);
  _vmll_deinit(&vmll, nres);
  return err;
}

candy_err_t candy_vm_pop(candy_vm_t *self, candy_excep_t *ctx, size_t n) {
  candy_err_t err = CANDY_OK;
  _vm_pop(self, ctx, n);
  return err;
}

candy_err_t candy_vm_push(candy_vm_t *self, candy_excep_t *ctx, const candy_wrap_t *wrap, size_t n) {
  candy_err_t err = CANDY_OK;
  _vm_push(self, ctx, wrap, n);
  return err;
}

const candy_wrap_t *candy_vm_view(candy_vm_t *self, candy_excep_t *ctx, int idx) {
  const candy_wrap_t *wrap = _vm_view(self, ctx, idx);
  return wrap;
}
