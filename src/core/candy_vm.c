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
#include <math.h>

#define vm_assert(_condition, _format, ...) \
candy_assert(ctx, self->gc, _condition, CANDY_ERR_VM, _format, ##__VA_ARGS__)

#if defined(NDEBUG)
#define vmll_assert(_condition, _format, ...) ((void)sizeof(_condition))
#else
#define vmll_assert(_condition, _format, ...) \
candy_assert(self->ctx, self->vm->gc, _condition, CANDY_ERR_VM, _format, ##__VA_ARGS__)
#endif

#if CANDY_CONFIG_VM_USE_JMPTABLE
#define vmdispatch(_opcode) goto *dispatch_table[(uint32_t)(_opcode)]; if (0)
#define vmcase(_opcode)     L_##_opcode
#define vmbreak()           do { \
  it = instructions + pc++; \
  vmdispatch(it->op) {} \
} while (0)
#else
#define vmdispatch(_opcode) switch ((candy_opcodes_t)(_opcode))
#define vmcase(_opcode)     case OP_##_opcode
#define vmbreak()           break
#endif /* CANDY_CONFIG_VM_USE_JMPTABLE */

typedef struct vmll vmll_t;

struct vmll {
  candy_callinfo_t *ci;
  uint8_t nlocal;
  candy_callinfo_t *prev;
  ptrdiff_t bos;
  candy_excep_t *ctx;
  candy_state_t *co;
  candy_vm_t *vm;
  candy_wrap_t *stack;
  const candy_vector_t *inst;
  const candy_wrap_t *constants;
  candy_inst_t *instructions;
  size_t inst_count;
};

/* Integer operands are the common case in hot loops; the float paths live in
 * one out-of-line helper so the dispatch loop stays small. */
typedef enum candy_arith {
  CANDY_ARITH_ADD,
  CANDY_ARITH_SUB,
  CANDY_ARITH_MUL,
  CANDY_ARITH_DIV,
  CANDY_ARITH_MOD,
  CANDY_ARITH_LT,
  CANDY_ARITH_LE,
  CANDY_ARITH_GT,
  CANDY_ARITH_GE,
} candy_arith_t;

static CANDY_HOT void _execute(vmll_t *self);

/* Grow a value/frame vector on demand, doubling from the minimum size; the
 * stack and frames start small and grow like Lua's stack. */
static CANDY_FORCE_INLINE void _vector_grow(candy_vector_t *vec, candy_memory_t *mem, candy_excep_t *ctx, size_t need, size_t cell) {
  size_t cap = candy_vector_size(vec);
  if (need <= cap)
    return;
  size_t next = cap ? cap : CANDY_CONFIG_VM_STACK_MIN;
  do {
    next *= 2;
  } while (next < need);
  candy_assert(ctx, NULL, next <= CANDY_CONFIG_VM_STACK_MAX, CANDY_ERR_VM, "stack overflow");
  size_t old = cap;
  candy_vector_resize(vec, mem, ctx, next, cell);
  memset((char *)candy_vector_data(vec) + old * cell, 0, (next - old) * cell);
}

static CANDY_FORCE_INLINE void _vm_push(candy_vm_t *self, candy_excep_t *ctx, const candy_wrap_t *wrap, ptrdiff_t n) {
  candy_callinfo_t *ci = self->ci;
  _vector_grow(&self->s, candy_gc_memory(self->gc), ctx, (size_t)(ci->tos + n), sizeof(candy_wrap_t));
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

static CANDY_NOINLINE void _vmll_gc_slow(vmll_t *self) {
  candy_gc_t *gc = self->vm->gc;
  candy_err_t err = candy_gc_full(gc);
  candy_assert(self->ctx, gc, err == CANDY_OK, CANDY_ERR_VM, "garbage collection failed");
}

static CANDY_FORCE_INLINE void _vmll_gc(vmll_t *self) {
  if (CANDY_LIKELY(candy_memory_used(candy_gc_memory(self->vm->gc)) < self->vm->gc->threshold))
    return;
  _vmll_gc_slow(self);
}

static CANDY_FORCE_INLINE candy_callinfo_t *_next_ci(vmll_t *self) {
  candy_vm_t *vm = self->vm;
  if (vm->ci->next == NULL) {
    candy_callinfo_t *ci = (candy_callinfo_t *)candy_gc_alloc(vm->gc, self->ctx, sizeof(candy_callinfo_t));
    memset(ci, 0, sizeof(candy_callinfo_t));
    vm->ci->next = ci;
    ci->prev = vm->ci;
  }
  return vm->ci->next;
}

static CANDY_FORCE_INLINE void _load_frame(vmll_t *self, const candy_vector_t *inst, const candy_wrap_t *constants, uint8_t nlocal) {
  self->nlocal = nlocal;
  self->inst = inst;
  self->instructions = (candy_inst_t *)candy_vector_data(inst);
  self->inst_count = candy_vector_size(inst);
  self->constants = constants;
}

static CANDY_FORCE_INLINE void _load_proto(vmll_t *self, const candy_proto_t *proto) {
  _load_frame(self, candy_proto_get_inst(proto), (const candy_wrap_t *)candy_vector_data(candy_proto_get_const(proto)), candy_proto_get_nlocal(proto));
}


/* Push a script-call frame. The function already occupies base[a] and its
 * arguments base[a+1..a+b]; the new frame's base is base + a. */
static CANDY_FORCE_INLINE void _frame_push(vmll_t *self, size_t return_pc, uint32_t a, uint32_t b) {
  candy_vm_t *vm = self->vm;
  candy_callinfo_t *parent = vm->ci;
  parent->pc = return_pc;
  const candy_wrap_t *fn = self->stack + parent->bos + a;
  const candy_proto_t *proto = candy_sclosure_get_proto((candy_sclosure_t *)candy_wrap_get_object(fn));
  uint8_t nreg = candy_proto_get_nreg(proto);
  candy_callinfo_t *frame = _next_ci(self);
  frame->bos = parent->bos + a;
  frame->tos = frame->bos + nreg;
  frame->inst = candy_proto_get_inst(proto);
  frame->constants = (const candy_wrap_t *)candy_vector_data(candy_proto_get_const(proto));
  _vector_grow(&vm->s, candy_gc_memory(vm->gc), self->ctx,
    (size_t)frame->bos + nreg, sizeof(candy_wrap_t));
  self->stack = (candy_wrap_t *)candy_vector_data(&vm->s);
  candy_wrap_t *base = self->stack + frame->bos;
  /* Temporaries are always written before they are read, so only the local
   * window has to start as none. */
  uint8_t nlocal = candy_proto_get_nlocal(proto);
  for (uint32_t i = b + 1; i < nlocal; ++i)
    candy_wrap_set_none(base + i);
  vm->ci = frame;
  self->ci = frame;
  _load_proto(self, proto);
}

/* Push a frame for a statically resolved call to the current function. */
static CANDY_FORCE_INLINE void _frame_push_self(vmll_t *self, size_t return_pc, uint32_t a, uint32_t b) {
  candy_vm_t *vm = self->vm;
  candy_callinfo_t *parent = vm->ci;
  parent->pc = return_pc;
  uint8_t nreg = (uint8_t)(parent->tos - parent->bos);
  candy_callinfo_t *frame = _next_ci(self);
  frame->bos = parent->bos + a;
  frame->tos = frame->bos + nreg;
  frame->inst = self->inst;
  frame->constants = self->constants;
  _vector_grow(&vm->s, candy_gc_memory(vm->gc), self->ctx,
    (size_t)frame->bos + nreg, sizeof(candy_wrap_t));
  self->stack = (candy_wrap_t *)candy_vector_data(&vm->s);
  /* The callee is the caller's own closure, so the slot already holds it; a
   * self call keeps its arguments in place and never overwrites it. */
  candy_wrap_t *base = self->stack + frame->bos;
  for (uint32_t i = b + 1; i < self->nlocal; ++i)
    candy_wrap_set_none(base + i);
  vm->ci = frame;
  self->ci = frame;
}

/* Finish the current frame: move ci back to the parent, or report that the
 * top-level frame finished. */
static CANDY_FORCE_INLINE bool _frame_return(vmll_t *self, size_t *pc) {
  candy_vm_t *vm = self->vm;
  candy_callinfo_t *frame = vm->ci->prev;
  vm->ci = frame;
  self->ci = frame;
  /* Unwinding into the frame this _execute was entered from ends it: that is
   * the outermost call, or a C function that called back into the VM. */
  if (frame == self->prev)
    return false;
  *pc = frame->pc;
  self->inst = frame->inst;
  self->instructions = (candy_inst_t *)candy_vector_data(self->inst);
  self->inst_count = candy_vector_size(self->inst);
  self->constants = frame->constants;
  return true;
}

/* Call a C function at base[a]; arguments follow the function slot. Results
 * land at base[a..a+res-1] and res is returned. */
static CANDY_FORCE_INLINE int _call_cfunction(vmll_t *self, candy_wrap_t *base, uint32_t a) {
  candy_vm_t *vm = self->vm;
  candy_cfunc_t cfunc = candy_wrap_get_cfunc(base + a);
  candy_callinfo_t cci;
  memset(&cci, 0, sizeof(cci));
  cci.bos = (base - self->stack) + a + 1;
  cci.tos = vm->ci->tos;
  ptrdiff_t saved_tos = cci.tos;
  candy_callinfo_t *saved = vm->ci;
  vm->ci = &cci;
  int res = cfunc(self->co);
  vm->ci = saved;
  for (int i = 0; i < res; ++i)
    base[a + i] = self->stack[saved_tos + i];
  return res;
}

static CANDY_FORCE_INLINE void _call_sclosure(vmll_t *self, const candy_wrap_t *fn) {
  candy_vm_t *vm = self->vm;
  const candy_proto_t *proto = candy_sclosure_get_proto((candy_sclosure_t *)candy_wrap_get_object(fn));
  uint8_t nreg = candy_proto_get_nreg(proto);
  candy_callinfo_t *frame = _next_ci(self);
  frame->bos = self->bos;
  frame->tos = self->bos + nreg;
  frame->pc = 0;
  frame->inst = candy_proto_get_inst(proto);
  frame->constants = (const candy_wrap_t *)candy_vector_data(candy_proto_get_const(proto));
  _vector_grow(&vm->s, candy_gc_memory(vm->gc), self->ctx, (size_t)self->bos + nreg, sizeof(candy_wrap_t));
  self->stack = (candy_wrap_t *)candy_vector_data(&vm->s);
  candy_wrap_t *base = self->stack + self->bos;
  uint8_t nlocal = candy_proto_get_nlocal(proto);
  for (uint32_t i = (uint32_t)(self->prev->tos - self->bos); i < nlocal; ++i)
    candy_wrap_set_none(base + i);
  vm->ci = frame;
  self->ci = frame;
  _load_proto(self, proto);
  _execute(self);
}

static CANDY_FORCE_INLINE void _call(vmll_t *self) {
  const candy_wrap_t *fn = self->stack + self->bos;
  switch (candy_wrap_type(fn)) {
    case CANDY_TYPE_CFUNC:
      _call_cfunction(self, self->stack + self->bos, 0);
      break;
    case CANDY_TYPE_SCLOS:
      _call_sclosure(self, fn);
      break;
    default:
      vmll_assert(false, "function expected");
      break;
  }
}

static CANDY_FORCE_INLINE void _op_move(vmll_t *self, const candy_inst_t *it, candy_wrap_t *base) {
  (void)self;
  base[it->iabx.a] = base[it->iabx.b];
}

static CANDY_FORCE_INLINE void _op_loadk(vmll_t *self, const candy_inst_t *it, candy_wrap_t *base, const candy_wrap_t *k) {
  (void)self;
  base[it->iabx.a] = k[it->iabx.b];
}

static CANDY_FORCE_INLINE void _op_loadg(vmll_t *self, const candy_inst_t *it, candy_wrap_t *base, const candy_wrap_t *k) {
  const candy_wrap_t *key = k + it->iabx.b;
  const candy_wrap_t *val = candy_table_get(candy_gc_global(self->vm->gc), self->vm->gc, key);
  base[it->iabx.a] = *val;
}

static CANDY_FORCE_INLINE void _op_setg(vmll_t *self, const candy_inst_t *it, candy_wrap_t *base, const candy_wrap_t *k) {
  const candy_wrap_t *key = k + it->iabx.b;
  const candy_wrap_t *val = base + it->iabx.a;
  candy_err_t err = candy_table_set(candy_gc_global(self->vm->gc), self->vm->gc, self->ctx, key, val);
  candy_assert(self->ctx, self->vm->gc, err == CANDY_OK, CANDY_ERR_VM, "set global failed");
}

static CANDY_FORCE_INLINE void _op_table(vmll_t *self, const candy_inst_t *it, candy_wrap_t *base) {
  candy_table_t *table = candy_table_create(self->vm->gc, self->ctx);
  candy_wrap_set_object(base + it->iax.a, (candy_object_t *)table);
  _vmll_gc(self);
}

static CANDY_FORCE_INLINE void _op_array(vmll_t *self, const candy_inst_t *it, candy_wrap_t *base) {
  candy_array_t *array = candy_array_create(self->vm->gc, self->ctx, CANDY_TYPE_WRAP);
  candy_wrap_set_object(base + it->iax.a, (candy_object_t *)array);
  _vmll_gc(self);
}

static CANDY_FORCE_INLINE void _op_gettable(vmll_t *self, const candy_inst_t *it, candy_wrap_t *base) {
  const candy_wrap_t *container = base + it->iabc.b;
  const candy_wrap_t *key = base + it->iabc.c;
  if (candy_wrap_type(container) == CANDY_TYPE_WRAP) {
    vmll_assert(candy_wrap_type(key) == CANDY_TYPE_INTEGER, "integer index expected");
    const candy_array_t *array = (const candy_array_t *)candy_wrap_get_object(container);
    base[it->iabc.a] = *candy_array_get(array, (size_t)candy_wrap_get_integer(key));
  }
  else {
    vmll_assert(candy_wrap_type(container) == CANDY_TYPE_TABLE, "table expected");
    const candy_wrap_t *val = candy_table_get((candy_table_t *)candy_wrap_get_object(container), self->vm->gc, key);
    base[it->iabc.a] = *val;
  }
}

static CANDY_FORCE_INLINE void _op_settable(vmll_t *self, const candy_inst_t *it, candy_wrap_t *base) {
  const candy_wrap_t *container = base + it->iabc.a;
  const candy_wrap_t *key = base + it->iabc.b;
  const candy_wrap_t *val = base + it->iabc.c;
  candy_err_t err = CANDY_OK;
  if (candy_wrap_type(container) == CANDY_TYPE_WRAP) {
    vmll_assert(candy_wrap_type(key) == CANDY_TYPE_INTEGER, "integer index expected");
    err = candy_array_set((candy_array_t *)candy_wrap_get_object(container), self->vm->gc, self->ctx,
      (size_t)candy_wrap_get_integer(key), val);
  }
  else {
    vmll_assert(candy_wrap_type(container) == CANDY_TYPE_TABLE, "table expected");
    err = candy_table_set((candy_table_t *)candy_wrap_get_object(container), self->vm->gc, self->ctx, key, val);
  }
  candy_assert(self->ctx, self->vm->gc, err == CANDY_OK, CANDY_ERR_VM, "set failed");
}

static CANDY_FORCE_INLINE void _op_setlist(vmll_t *self, const candy_inst_t *it, candy_wrap_t *base, const candy_wrap_t *k) {
  const candy_wrap_t *container = base + it->iabc.a;
  candy_integer_t count = candy_wrap_get_integer(base + it->iabc.b);
  candy_integer_t start = candy_wrap_get_integer(base + it->iabc.c);
  candy_err_t err = CANDY_OK;
  if (candy_wrap_type(container) == CANDY_TYPE_WRAP) {
    candy_array_t *array = (candy_array_t *)candy_wrap_get_object(container);
    err = candy_array_append(array, self->vm->gc, self->ctx, k + start, (size_t)count);
  }
  else {
    candy_table_t *table = (candy_table_t *)candy_wrap_get_object(container);
    for (candy_integer_t index = 0; index < count && err == CANDY_OK; ++index) {
      candy_wrap_t key;
      candy_wrap_set_integer(&key, index);
      err = candy_table_set(table, self->vm->gc, self->ctx, &key, k + start + index);
    }
  }
  candy_assert(self->ctx, self->vm->gc, err == CANDY_OK, CANDY_ERR_VM, "setlist failed");
}

static CANDY_FORCE_INLINE void _op_len(vmll_t *self, const candy_inst_t *it, candy_wrap_t *base) {
  const candy_wrap_t *container = base + it->iabx.b;
  size_t size = 0;
  if (candy_wrap_type(container) == CANDY_TYPE_WRAP)
    size = candy_array_size((const candy_array_t *)candy_wrap_get_object(container));
  else {
    vmll_assert(candy_wrap_type(container) == CANDY_TYPE_TABLE, "table expected");
    size = candy_table_size((candy_table_t *)candy_wrap_get_object(container), self->vm->gc);
  }
  candy_wrap_set_integer(base + it->iabx.a, (candy_integer_t)size);
}

static CANDY_FORCE_INLINE void _op_forloop(vmll_t *self, const candy_inst_t *it, candy_wrap_t *base, size_t *pc) {
  (void)self;
  candy_wrap_t *idx = base + it->iabx.a;
  const candy_wrap_t *len = idx + 1;
  candy_integer_t next = candy_wrap_get_integer(idx) + 1;
  candy_wrap_set_integer(idx, next);
  if (next < candy_wrap_get_integer(len))
    *pc = it->iabx.b;
}

static CANDY_FORCE_INLINE bool _both_integer(const candy_wrap_t *lhs, const candy_wrap_t *rhs) {
  return candy_wrap_type(lhs) == CANDY_TYPE_INTEGER && candy_wrap_type(rhs) == CANDY_TYPE_INTEGER;
}

static CANDY_NOINLINE void _arith_slow(candy_wrap_t *dst, const candy_wrap_t *lhs, const candy_wrap_t *rhs, candy_arith_t op) {
  candy_float_t a = candy_wrap_to_float(lhs);
  candy_float_t b = candy_wrap_to_float(rhs);
  switch (op) {
    case CANDY_ARITH_ADD: candy_wrap_set_float(dst, a + b); break;
    case CANDY_ARITH_SUB: candy_wrap_set_float(dst, a - b); break;
    case CANDY_ARITH_MUL: candy_wrap_set_float(dst, a * b); break;
    case CANDY_ARITH_DIV: candy_wrap_set_float(dst, a / b); break;
    case CANDY_ARITH_MOD: candy_wrap_set_float(dst, fmod(a, b)); break;
    case CANDY_ARITH_LT:  candy_wrap_set_boolean(dst, a < b); break;
    case CANDY_ARITH_LE:  candy_wrap_set_boolean(dst, a <= b); break;
    case CANDY_ARITH_GT:  candy_wrap_set_boolean(dst, a > b); break;
    case CANDY_ARITH_GE:  candy_wrap_set_boolean(dst, a >= b); break;
  }
}

static CANDY_FORCE_INLINE void _op_add(const candy_inst_t *it, candy_wrap_t *base) {
  candy_wrap_t *dst = base + it->iabc.a;
  const candy_wrap_t *lhs = base + it->iabc.b;
  const candy_wrap_t *rhs = base + it->iabc.c;
  if (CANDY_LIKELY(_both_integer(lhs, rhs)))
    candy_wrap_set_integer(dst, candy_wrap_get_integer(lhs) + candy_wrap_get_integer(rhs));
  else
    _arith_slow(dst, lhs, rhs, CANDY_ARITH_ADD);
}

static CANDY_FORCE_INLINE void _op_addg(vmll_t *self, const candy_inst_t *it, candy_wrap_t *base, const candy_wrap_t *k) {
  candy_table_t *globals = candy_gc_global(self->vm->gc);
  const candy_wrap_t *key = k + it->iabc.b;
  const candy_wrap_t *cur = candy_table_get(globals, self->vm->gc, key);
  const candy_wrap_t *rhs = base + it->iabc.c;
  candy_wrap_t sum;
  if (CANDY_LIKELY(_both_integer(cur, rhs)))
    candy_wrap_set_integer(&sum, candy_wrap_get_integer(cur) + candy_wrap_get_integer(rhs));
  else
    _arith_slow(&sum, cur, rhs, CANDY_ARITH_ADD);
  candy_err_t err = candy_table_set(globals, self->vm->gc, self->ctx, key, &sum);
  candy_assert(self->ctx, self->vm->gc, err == CANDY_OK, CANDY_ERR_VM, "set global failed");
}

static CANDY_FORCE_INLINE void _op_addgk(vmll_t *self, const candy_inst_t *it, const candy_wrap_t *k) {
  candy_table_t *globals = candy_gc_global(self->vm->gc);
  const candy_wrap_t *key = k + it->iabc.b;
  const candy_wrap_t *cur = candy_table_get(globals, self->vm->gc, key);
  const candy_wrap_t *rhs = k + it->iabc.c;
  candy_wrap_t sum;
  if (CANDY_LIKELY(_both_integer(cur, rhs)))
    candy_wrap_set_integer(&sum, candy_wrap_get_integer(cur) + candy_wrap_get_integer(rhs));
  else
    _arith_slow(&sum, cur, rhs, CANDY_ARITH_ADD);
  candy_err_t err = candy_table_set(globals, self->vm->gc, self->ctx, key, &sum);
  candy_assert(self->ctx, self->vm->gc, err == CANDY_OK, CANDY_ERR_VM, "set global failed");
}

static CANDY_FORCE_INLINE void _op_addk(vmll_t *self, const candy_inst_t *it, candy_wrap_t *base, const candy_wrap_t *k) {
  (void)self;
  candy_wrap_t *dst = base + it->iabc.a;
  const candy_wrap_t *lhs = base + it->iabc.b;
  const candy_wrap_t *rhs = k + it->iabc.c;
  if (CANDY_LIKELY(_both_integer(lhs, rhs)))
    candy_wrap_set_integer(dst, candy_wrap_get_integer(lhs) + candy_wrap_get_integer(rhs));
  else
    _arith_slow(dst, lhs, rhs, CANDY_ARITH_ADD);
}

static CANDY_FORCE_INLINE void _op_sub(vmll_t *self, const candy_inst_t *it, candy_wrap_t *base) {
  (void)self;
  candy_wrap_t *dst = base + it->iabc.a;
  const candy_wrap_t *lhs = base + it->iabc.b;
  const candy_wrap_t *rhs = base + it->iabc.c;
  if (CANDY_LIKELY(_both_integer(lhs, rhs)))
    candy_wrap_set_integer(dst, candy_wrap_get_integer(lhs) - candy_wrap_get_integer(rhs));
  else
    _arith_slow(dst, lhs, rhs, CANDY_ARITH_SUB);
}

static CANDY_FORCE_INLINE void _op_subk(vmll_t *self, const candy_inst_t *it, candy_wrap_t *base, const candy_wrap_t *k) {
  (void)self;
  candy_wrap_t *dst = base + it->iabc.a;
  const candy_wrap_t *lhs = base + it->iabc.b;
  const candy_wrap_t *rhs = k + it->iabc.c;
  if (CANDY_LIKELY(_both_integer(lhs, rhs)))
    candy_wrap_set_integer(dst, candy_wrap_get_integer(lhs) - candy_wrap_get_integer(rhs));
  else
    _arith_slow(dst, lhs, rhs, CANDY_ARITH_SUB);
}

static CANDY_FORCE_INLINE void _op_mul(vmll_t *self, const candy_inst_t *it, candy_wrap_t *base) {
  (void)self;
  candy_wrap_t *dst = base + it->iabc.a;
  const candy_wrap_t *lhs = base + it->iabc.b;
  const candy_wrap_t *rhs = base + it->iabc.c;
  if (CANDY_LIKELY(_both_integer(lhs, rhs)))
    candy_wrap_set_integer(dst, candy_wrap_get_integer(lhs) * candy_wrap_get_integer(rhs));
  else
    _arith_slow(dst, lhs, rhs, CANDY_ARITH_MUL);
}

static CANDY_FORCE_INLINE void _op_mulk(vmll_t *self, const candy_inst_t *it, candy_wrap_t *base, const candy_wrap_t *k) {
  (void)self;
  candy_wrap_t *dst = base + it->iabc.a;
  const candy_wrap_t *lhs = base + it->iabc.b;
  const candy_wrap_t *rhs = k + it->iabc.c;
  if (CANDY_LIKELY(_both_integer(lhs, rhs)))
    candy_wrap_set_integer(dst, candy_wrap_get_integer(lhs) * candy_wrap_get_integer(rhs));
  else
    _arith_slow(dst, lhs, rhs, CANDY_ARITH_MUL);
}

static CANDY_FORCE_INLINE void _op_div(vmll_t *self, const candy_inst_t *it, candy_wrap_t *base) {
  (void)self;
  candy_wrap_t *dst = base + it->iabc.a;
  const candy_wrap_t *lhs = base + it->iabc.b;
  const candy_wrap_t *rhs = base + it->iabc.c;
  if (CANDY_LIKELY(_both_integer(lhs, rhs)))
    candy_wrap_set_integer(dst, candy_wrap_get_integer(lhs) / candy_wrap_get_integer(rhs));
  else
    _arith_slow(dst, lhs, rhs, CANDY_ARITH_DIV);
}

static CANDY_FORCE_INLINE void _op_divk(vmll_t *self, const candy_inst_t *it, candy_wrap_t *base, const candy_wrap_t *k) {
  (void)self;
  candy_wrap_t *dst = base + it->iabc.a;
  const candy_wrap_t *lhs = base + it->iabc.b;
  const candy_wrap_t *rhs = k + it->iabc.c;
  if (CANDY_LIKELY(_both_integer(lhs, rhs)))
    candy_wrap_set_integer(dst, candy_wrap_get_integer(lhs) / candy_wrap_get_integer(rhs));
  else
    _arith_slow(dst, lhs, rhs, CANDY_ARITH_DIV);
}

static CANDY_FORCE_INLINE void _op_mod(vmll_t *self, const candy_inst_t *it, candy_wrap_t *base) {
  (void)self;
  candy_wrap_t *dst = base + it->iabc.a;
  const candy_wrap_t *lhs = base + it->iabc.b;
  const candy_wrap_t *rhs = base + it->iabc.c;
  if (CANDY_LIKELY(_both_integer(lhs, rhs)))
    candy_wrap_set_integer(dst, candy_wrap_get_integer(lhs) % candy_wrap_get_integer(rhs));
  else
    _arith_slow(dst, lhs, rhs, CANDY_ARITH_MOD);
}

static CANDY_FORCE_INLINE void _op_modk(vmll_t *self, const candy_inst_t *it, candy_wrap_t *base, const candy_wrap_t *k) {
  (void)self;
  candy_wrap_t *dst = base + it->iabc.a;
  const candy_wrap_t *lhs = base + it->iabc.b;
  const candy_wrap_t *rhs = k + it->iabc.c;
  if (CANDY_LIKELY(_both_integer(lhs, rhs)))
    candy_wrap_set_integer(dst, candy_wrap_get_integer(lhs) % candy_wrap_get_integer(rhs));
  else
    _arith_slow(dst, lhs, rhs, CANDY_ARITH_MOD);
}

static CANDY_FORCE_INLINE void _cmp_lt(candy_wrap_t *dst, const candy_wrap_t *lhs, const candy_wrap_t *rhs) {
  if (CANDY_LIKELY(_both_integer(lhs, rhs)))
    candy_wrap_set_boolean(dst, candy_wrap_get_integer(lhs) < candy_wrap_get_integer(rhs));
  else
    _arith_slow(dst, lhs, rhs, CANDY_ARITH_LT);
}

static CANDY_FORCE_INLINE void _cmp_le(candy_wrap_t *dst, const candy_wrap_t *lhs, const candy_wrap_t *rhs) {
  if (CANDY_LIKELY(_both_integer(lhs, rhs)))
    candy_wrap_set_boolean(dst, candy_wrap_get_integer(lhs) <= candy_wrap_get_integer(rhs));
  else
    _arith_slow(dst, lhs, rhs, CANDY_ARITH_LE);
}

static CANDY_FORCE_INLINE void _cmp_gt(candy_wrap_t *dst, const candy_wrap_t *lhs, const candy_wrap_t *rhs) {
  if (CANDY_LIKELY(_both_integer(lhs, rhs)))
    candy_wrap_set_boolean(dst, candy_wrap_get_integer(lhs) > candy_wrap_get_integer(rhs));
  else
    _arith_slow(dst, lhs, rhs, CANDY_ARITH_GT);
}

static CANDY_FORCE_INLINE void _cmp_ge(candy_wrap_t *dst, const candy_wrap_t *lhs, const candy_wrap_t *rhs) {
  if (CANDY_LIKELY(_both_integer(lhs, rhs)))
    candy_wrap_set_boolean(dst, candy_wrap_get_integer(lhs) >= candy_wrap_get_integer(rhs));
  else
    _arith_slow(dst, lhs, rhs, CANDY_ARITH_GE);
}

static CANDY_FORCE_INLINE void _op_lt(vmll_t *self, const candy_inst_t *it, candy_wrap_t *base) {
  (void)self;
  _cmp_lt(base + it->iabc.a, base + it->iabc.b, base + it->iabc.c);
}

static CANDY_FORCE_INLINE void _op_ltk(vmll_t *self, const candy_inst_t *it, candy_wrap_t *base, const candy_wrap_t *k) {
  (void)self;
  _cmp_lt(base + it->iabc.a, base + it->iabc.b, k + it->iabc.c);
}

static CANDY_FORCE_INLINE void _op_eq(const candy_inst_t *it, candy_wrap_t *base) {
  candy_wrap_set_boolean(base + it->iabc.a, candy_wrap_equal(base + it->iabc.b, base + it->iabc.c));
}

static CANDY_FORCE_INLINE void _op_eqk(const candy_inst_t *it, candy_wrap_t *base, const candy_wrap_t *k) {
  candy_wrap_set_boolean(base + it->iabc.a, candy_wrap_equal(base + it->iabc.b, k + it->iabc.c));
}

static CANDY_FORCE_INLINE void _op_neq(const candy_inst_t *it, candy_wrap_t *base) {
  candy_wrap_set_boolean(base + it->iabc.a, !candy_wrap_equal(base + it->iabc.b, base + it->iabc.c));
}

static CANDY_FORCE_INLINE void _op_neqk(const candy_inst_t *it, candy_wrap_t *base, const candy_wrap_t *k) {
  candy_wrap_set_boolean(base + it->iabc.a, !candy_wrap_equal(base + it->iabc.b, k + it->iabc.c));
}

static CANDY_FORCE_INLINE bool _truthy(const candy_wrap_t *w) {
  candy_types_t type = candy_wrap_type(w);
  if (type == CANDY_TYPE_BOOLEAN)
    return candy_wrap_get_boolean(w);
  if (type == CANDY_TYPE_NULL)
    return false;
  return true;
}

static CANDY_FORCE_INLINE void _op_bool(const candy_inst_t *it, candy_wrap_t *base) {
  candy_wrap_set_boolean(base + it->iabc.a, _truthy(base + it->iabc.b));
}

static CANDY_FORCE_INLINE void _op_not(const candy_inst_t *it, candy_wrap_t *base) {
  candy_wrap_set_boolean(base + it->iabc.a, !_truthy(base + it->iabc.b));
}

static CANDY_FORCE_INLINE void _op_le(const candy_inst_t *it, candy_wrap_t *base) {
  _cmp_le(base + it->iabc.a, base + it->iabc.b, base + it->iabc.c);
}

static CANDY_FORCE_INLINE void _op_lek(const candy_inst_t *it, candy_wrap_t *base, const candy_wrap_t *k) {
  _cmp_le(base + it->iabc.a, base + it->iabc.b, k + it->iabc.c);
}

static CANDY_FORCE_INLINE void _op_gt(const candy_inst_t *it, candy_wrap_t *base) {
  _cmp_gt(base + it->iabc.a, base + it->iabc.b, base + it->iabc.c);
}

static CANDY_FORCE_INLINE void _op_gtk(const candy_inst_t *it, candy_wrap_t *base, const candy_wrap_t *k) {
  _cmp_gt(base + it->iabc.a, base + it->iabc.b, k + it->iabc.c);
}

static CANDY_FORCE_INLINE void _op_ge(const candy_inst_t *it, candy_wrap_t *base) {
  _cmp_ge(base + it->iabc.a, base + it->iabc.b, base + it->iabc.c);
}

static CANDY_FORCE_INLINE void _op_gek(const candy_inst_t *it, candy_wrap_t *base, const candy_wrap_t *k) {
  _cmp_ge(base + it->iabc.a, base + it->iabc.b, k + it->iabc.c);
}

static CANDY_FORCE_INLINE void _bit_op(vmll_t *self, const candy_inst_t *it, candy_wrap_t *base, const candy_wrap_t *rhs, candy_integer_t (*fn)(candy_integer_t, candy_integer_t)) {
  vmll_assert(candy_wrap_type(base + it->iabc.b) == CANDY_TYPE_INTEGER && candy_wrap_type(rhs) == CANDY_TYPE_INTEGER, "integer expected");
  candy_wrap_set_integer(base + it->iabc.a, fn(candy_wrap_get_integer(base + it->iabc.b), candy_wrap_get_integer(rhs)));
}

static CANDY_FORCE_INLINE candy_integer_t _i_band(candy_integer_t a, candy_integer_t b) {
  return a & b;
}

static CANDY_FORCE_INLINE candy_integer_t _i_bor(candy_integer_t a, candy_integer_t b) {
  return a | b;
}

static CANDY_FORCE_INLINE candy_integer_t _i_bxor(candy_integer_t a, candy_integer_t b) {
  return a ^ b;
}

static CANDY_FORCE_INLINE candy_integer_t _i_shl(candy_integer_t a, candy_integer_t b) {
  return a << b;
}

static CANDY_FORCE_INLINE candy_integer_t _i_shr(candy_integer_t a, candy_integer_t b) {
  return a >> b;
}

static CANDY_FORCE_INLINE void _op_band(vmll_t *self, const candy_inst_t *it, candy_wrap_t *base) {
  _bit_op(self, it, base, base + it->iabc.c, _i_band);
}

static CANDY_FORCE_INLINE void _op_bandk(vmll_t *self, const candy_inst_t *it, candy_wrap_t *base, const candy_wrap_t *k) {
  _bit_op(self, it, base, k + it->iabc.c, _i_band);
}

static CANDY_FORCE_INLINE void _op_bor(vmll_t *self, const candy_inst_t *it, candy_wrap_t *base) {
  _bit_op(self, it, base, base + it->iabc.c, _i_bor);
}

static CANDY_FORCE_INLINE void _op_bork(vmll_t *self, const candy_inst_t *it, candy_wrap_t *base, const candy_wrap_t *k) {
  _bit_op(self, it, base, k + it->iabc.c, _i_bor);
}

static CANDY_FORCE_INLINE void _op_bxor(vmll_t *self, const candy_inst_t *it, candy_wrap_t *base) {
  _bit_op(self, it, base, base + it->iabc.c, _i_bxor);
}

static CANDY_FORCE_INLINE void _op_bxork(vmll_t *self, const candy_inst_t *it, candy_wrap_t *base, const candy_wrap_t *k) {
  _bit_op(self, it, base, k + it->iabc.c, _i_bxor);
}

static CANDY_FORCE_INLINE void _op_shl(vmll_t *self, const candy_inst_t *it, candy_wrap_t *base) {
  _bit_op(self, it, base, base + it->iabc.c, _i_shl);
}

static CANDY_FORCE_INLINE void _op_shlk(vmll_t *self, const candy_inst_t *it, candy_wrap_t *base, const candy_wrap_t *k) {
  _bit_op(self, it, base, k + it->iabc.c, _i_shl);
}

static CANDY_FORCE_INLINE void _op_shr(vmll_t *self, const candy_inst_t *it, candy_wrap_t *base) {
  _bit_op(self, it, base, base + it->iabc.c, _i_shr);
}

static CANDY_FORCE_INLINE void _op_shrk(vmll_t *self, const candy_inst_t *it, candy_wrap_t *base, const candy_wrap_t *k) {
  _bit_op(self, it, base, k + it->iabc.c, _i_shr);
}

static CANDY_FORCE_INLINE void _op_jmp(vmll_t *self, const candy_inst_t *it, size_t *pc) {
  (void)self;
  *pc = it->iax.a;
}

static CANDY_FORCE_INLINE void _op_jmpf(vmll_t *self, const candy_inst_t *it, candy_wrap_t *base, size_t *pc) {
  (void)self;
  vmll_assert(candy_wrap_type(base + it->iabx.a) == CANDY_TYPE_BOOLEAN, "boolean expected");
  if (CANDY_UNLIKELY(!candy_wrap_get_boolean(base + it->iabx.a)))
    *pc = it->iabx.b;
}

static CANDY_FORCE_INLINE void _op_jmpt(vmll_t *self, const candy_inst_t *it, candy_wrap_t *base, size_t *pc) {
  (void)self;
  vmll_assert(candy_wrap_type(base + it->iabc.a) == CANDY_TYPE_BOOLEAN, "boolean expected");
  if (CANDY_LIKELY(candy_wrap_get_boolean(base + it->iabc.a)))
    ++*pc;
}

static CANDY_FORCE_INLINE bool _op_call(vmll_t *self, const candy_inst_t *it, candy_wrap_t *base, size_t *pc) {
  uint32_t a = it->iabc.a;
  uint32_t b = it->iabc.b;
  uint32_t c = it->iabc.c;
  if (CANDY_LIKELY(candy_wrap_type(base + a) == CANDY_TYPE_SCLOS)) {
    if (candy_wrap_get_object(base + a) == candy_wrap_get_object(base))
      _frame_push_self(self, *pc, a, b);
    else
      _frame_push(self, *pc, a, b);
    *pc = 0;
    return true;
  }
  int res = _call_cfunction(self, base, a);
  for (uint32_t i = (uint32_t)res; i < c; ++i)
    base[a + i] = CANDY_WRAP_NULL;
  _vmll_gc(self);
  return false;
}

static CANDY_FORCE_INLINE void _op_callself(vmll_t *self, const candy_inst_t *it, size_t *pc) {
  _frame_push_self(self, *pc, it->iabc.a, it->iabc.b);
  *pc = 0;
}

static CANDY_FORCE_INLINE bool _op_return(vmll_t *self, const candy_inst_t *it, candy_wrap_t *base, size_t *pc) {
  uint32_t a = it->iax.a;
  if (a != 0)
    base[0] = base[a];
  return _frame_return(self, pc);
}

static CANDY_FORCE_INLINE bool _op_tailcall(vmll_t *self, const candy_inst_t *it, candy_wrap_t *base, size_t *pc) {
  uint32_t a = it->iabx.a;
  uint32_t b = it->iabx.b;
  const candy_wrap_t *fn = base + a;
  if (CANDY_LIKELY(candy_wrap_type(fn) == CANDY_TYPE_SCLOS)) {
    /* Capture the callee proto before the argument move overwrites its slot. */
    const candy_proto_t *proto = candy_sclosure_get_proto((candy_sclosure_t *)candy_wrap_get_object(fn));
    uint8_t nreg = candy_proto_get_nreg(proto);
    uint8_t nlocal = candy_proto_get_nlocal(proto);
    candy_wrap_t closure = *fn;
    if (a != 0) {
      memmove(base + 1, base + a + 1, b * sizeof(candy_wrap_t));
      base[0] = closure;
    }
    /* The callee may need a taller frame than the caller had: grow the value
     * stack (which can move it) before touching the registers. */
    candy_vm_t *vm = self->vm;
    if ((size_t)self->ci->bos + nreg > candy_vector_size(&vm->s)) {
      _vector_grow(&vm->s, candy_gc_memory(vm->gc), self->ctx, (size_t)self->ci->bos + nreg, sizeof(candy_wrap_t));
      self->stack = (candy_wrap_t *)candy_vector_data(&vm->s);
      base = self->stack + self->ci->bos;
    }
    for (uint32_t i = b + 1; i < nlocal; ++i)
      candy_wrap_set_none(base + i);
    self->ci->tos = self->ci->bos + nreg;
    const candy_vector_t *inst = candy_proto_get_inst(proto);
    const candy_wrap_t *constants = (const candy_wrap_t *)candy_vector_data(candy_proto_get_const(proto));
    _load_frame(self, inst, constants, nlocal);
    self->ci->inst = inst;
    self->ci->constants = constants;
    *pc = 0;
    return true;
  }
  if (a != 0)
    memmove(base, base + a, (b + 1) * sizeof(candy_wrap_t));
  int res = _call_cfunction(self, self->stack + self->ci->bos, 0);
  if (res < 1)
    base[0] = CANDY_WRAP_NULL;
  _vmll_gc(self);
  return _frame_return(self, pc);
}

static CANDY_FORCE_INLINE void _op_tailcallself(vmll_t *self, const candy_inst_t *it, candy_wrap_t *base, size_t *pc) {
  uint32_t a = it->iabx.a;
  uint32_t b = it->iabx.b;
  candy_wrap_t fn = base[0];
  if (a != 0)
    memmove(base + 1, base + a + 1, b * sizeof(candy_wrap_t));
  base[0] = fn;
  for (uint32_t i = b + 1; i < self->nlocal; ++i)
    candy_wrap_set_none(base + i);
  *pc = 0;
}

static CANDY_HOT __attribute__((aligned(64))) void _execute(vmll_t *self) {
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

  size_t pc = 0;
reload:
  for (;;) {
    candy_inst_t *instructions = self->instructions;
    const size_t inst_count = self->inst_count;
    candy_wrap_t *base = self->stack + self->ci->bos;
    const candy_wrap_t *constants = self->constants;
    while (pc < inst_count) {
      candy_inst_t *it = instructions + pc++;
      vmdispatch(it->op) {
        #define CANDY_OPCODE_CASE
        #include "core/candy_opcode.list"
      }
    }
    return; /* malformed proto without a trailing RETURN */
  }
}

static void _vmll_init(vmll_t *self, candy_vm_t *vm, candy_excep_t *ctx, int narg, candy_state_t *co) {
  self->ci = vm->ci;
  self->prev = vm->ci;
  self->bos = vm->ci->tos - narg - 1;
  self->ctx = ctx;
  self->co = co;
  self->vm = vm;
  self->stack = (candy_wrap_t *)candy_vector_data(&vm->s);
}

static CANDY_FORCE_INLINE void _vmll_deinit(vmll_t *self, int nres) {
  candy_vm_t *vm = self->vm;
  vm->ci = self->prev;
  self->prev->tos = self->bos + nres;
}

candy_err_t candy_vm_init(candy_vm_t *self, candy_gc_t *gc, candy_excep_t *ctx) {
  candy_err_t err = CANDY_OK;
  candy_vector_init(&self->s);
  candy_vector_resize(&self->s, candy_gc_memory(gc), ctx, CANDY_CONFIG_VM_STACK_MIN, sizeof(candy_wrap_t));
  memset(candy_vector_data(&self->s), 0, candy_vector_size(&self->s) * sizeof(candy_wrap_t));
  memset(&self->base, 0, sizeof(candy_callinfo_t));
  self->ci = &self->base;
  self->gc = gc;
  return err;
}

candy_err_t candy_vm_deinit(candy_vm_t *self) {
  candy_err_t err = CANDY_OK;
  self->ci = NULL;
  candy_callinfo_t *ci = self->base.next;
  while (ci != NULL) {
    candy_callinfo_t *next = ci->next;
    candy_gc_free(self->gc, ci, sizeof(candy_callinfo_t));
    ci = next;
  }
  self->base.next = NULL;
  candy_vector_deinit(&self->s, candy_gc_memory(self->gc), sizeof(candy_wrap_t));
  self->gc = NULL;
  return err;
}

candy_err_t candy_vm_call(candy_vm_t *self, candy_excep_t *ctx, int narg, int nres, candy_state_t *co, candy_object_t **out) {
  vmll_t vmll;
  _vmll_init(&vmll, self, ctx, narg, co);
  candy_err_t err = candy_excep_try(ctx, (candy_excep_cb_t)_call, &vmll, out);
  _vmll_deinit(&vmll, err == CANDY_OK ? nres : 0);
  return err;
}

candy_err_t candy_vm_push(candy_vm_t *self, candy_excep_t *ctx, const candy_wrap_t *wrap, size_t n) {
  candy_err_t err = CANDY_OK;
  _vm_push(self, ctx, wrap, n);
  if (candy_memory_used(candy_gc_memory(self->gc)) >= self->gc->threshold) {
    err = candy_gc_full(self->gc);
    candy_assert(ctx, self->gc, err == CANDY_OK, CANDY_ERR_VM, "garbage collection failed");
  }
  return err;
}

const candy_wrap_t *candy_vm_view(candy_vm_t *self, candy_excep_t *ctx, int idx) {
  const candy_wrap_t *wrap = _vm_view(self, ctx, idx);
  return wrap;
}
