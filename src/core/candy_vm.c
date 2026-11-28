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
#include "core/candy_upvalue.h"
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

#define vmtestorder(_compare) do { \
  const candy_wrap_t *rhs = (it->iabc.a ? constants : base) + it->iabc.c; \
  vmll_assert(pc < inst_count && instructions[pc].op == OP_JMP, "missing comparison branch target"); \
  pc = _compare(base + it->iabc.b, rhs) ? pc + 1 : instructions[pc].iax.a; \
} while (0)

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
};

/* Integer operands are the common case in hot loops; the float paths live in
 * one out-of-line helper so the dispatch loop stays small. */
typedef enum candy_arith {
  CANDY_ARITH_ADD,
  CANDY_ARITH_SUB,
  CANDY_ARITH_MUL,
  CANDY_ARITH_DIV,
  CANDY_ARITH_MOD,
} candy_arith_t;

static CANDY_HOT void _execute(vmll_t *self);

/* Reuse the current stack view unless a resize can invalidate it. */
static CANDY_FORCE_INLINE candy_wrap_t *_stack_reserve(candy_vm_t *vm, candy_excep_t *ctx, size_t need, candy_wrap_t *stack) {
  candy_vector_t *vec = &vm->s;
  size_t cap = candy_vector_size(vec);
  size_t next;
  if (need <= cap)
    return stack ? stack : (candy_wrap_t *)candy_vector_data(vec);
  next = cap ? cap : CANDY_CONFIG_VM_STACK_MIN;
  do {
    next *= 2;
  } while (next < need);
  candy_assert(ctx, NULL, next <= CANDY_CONFIG_VM_STACK_MAX, CANDY_ERR_VM, "stack overflow");
  candy_vector_resize(vec, candy_gc_memory(vm->gc), ctx, next, sizeof(candy_wrap_t));
  stack = (candy_wrap_t *)candy_vector_data(vec);
  memset(stack + cap, 0, (next - cap) * sizeof(candy_wrap_t));
  return stack;
}

static CANDY_FORCE_INLINE void _vm_push(candy_vm_t *self, candy_excep_t *ctx, const candy_wrap_t *wrap, ptrdiff_t n) {
  candy_callinfo_t *ci = self->ci;
  candy_wrap_t *stack = _stack_reserve(self, ctx, (size_t)(ci->tos + n), NULL);
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

static CANDY_NOINLINE candy_callinfo_t *_new_ci(vmll_t *self) {
  candy_vm_t *vm = self->vm;
  candy_callinfo_t *ci = (candy_callinfo_t *)candy_gc_alloc(vm->gc, self->ctx, sizeof(candy_callinfo_t));
  memset(ci, 0, sizeof(candy_callinfo_t));
  vm->ci->next = ci;
  ci->prev = vm->ci;
  return ci;
}

static CANDY_FORCE_INLINE candy_callinfo_t *_next_ci(vmll_t *self) {
  candy_callinfo_t *next = self->vm->ci->next;
  return CANDY_LIKELY(next != NULL) ? next : _new_ci(self);
}

static CANDY_FORCE_INLINE void _load_frame(vmll_t *self, const candy_vector_t *inst, const candy_wrap_t *constants, uint8_t nlocal) {
  self->nlocal = nlocal;
  self->inst = inst;
  self->constants = constants;
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
  frame->constructor = false;
  frame->bos = parent->bos + a;
  frame->tos = frame->bos + nreg;
  frame->inst = candy_proto_get_inst(proto);
  frame->constants = (const candy_wrap_t *)candy_vector_data(candy_proto_get_const(proto));
  self->stack = _stack_reserve(vm, self->ctx, (size_t)frame->tos, self->stack);
  candy_wrap_t *base = self->stack + frame->bos;
  /* Temporaries are always written before they are read, so only the local
   * window has to start as none. */
  uint8_t nlocal = candy_proto_get_nlocal(proto);
  for (uint32_t i = b + 1; i < nlocal; ++i)
    candy_wrap_set_none(base + i);
  vm->ci = frame;
  self->ci = frame;
  _load_frame(self, frame->inst, frame->constants, nlocal);
}

/* Push a frame for a statically resolved call to the current function. */
static CANDY_FORCE_INLINE void _frame_push_self(vmll_t *self, size_t return_pc, uint32_t a, uint32_t b) {
  candy_vm_t *vm = self->vm;
  candy_callinfo_t *parent = vm->ci;
  parent->pc = return_pc;
  uint8_t nreg = (uint8_t)(parent->tos - parent->bos);
  candy_callinfo_t *frame = _next_ci(self);
  frame->constructor = false;
  frame->bos = parent->bos + a;
  frame->tos = frame->bos + nreg;
  frame->inst = self->inst;
  frame->constants = self->constants;
  self->stack = _stack_reserve(vm, self->ctx, (size_t)frame->tos, self->stack);
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
  if (vm->open != NULL)
    candy_upvalue_close_all(&vm->open, vm->ci->bos);
  candy_callinfo_t *current = vm->ci;
  candy_callinfo_t *frame = current->prev;
  if (CANDY_UNLIKELY(current->constructor))
    self->stack[current->bos] = current->instance;
  vm->ci = frame;
  self->ci = frame;
  /* Unwinding into the frame this _execute was entered from ends it: that is
   * the outermost call, or a C function that called back into the VM. */
  if (frame == self->prev)
    return false;
  *pc = frame->pc;
  if (frame->inst != self->inst) {
    candy_sclosure_t *closure = (candy_sclosure_t *)candy_wrap_get_object(self->stack + frame->bos);
    const candy_proto_t *proto = candy_sclosure_get_proto(closure);
    _load_frame(self, frame->inst, frame->constants, candy_proto_get_nlocal(proto));
  }
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
  self->stack = (candy_wrap_t *)candy_vector_data(&vm->s);
  for (int i = 0; i < res; ++i)
    self->stack[cci.bos - 1 + i] = self->stack[saved_tos + i];
  return res;
}

static CANDY_FORCE_INLINE void _call_sclosure(vmll_t *self, const candy_proto_t *proto) {
  candy_vm_t *vm = self->vm;
  uint8_t nreg = candy_proto_get_nreg(proto);
  candy_callinfo_t *frame = _next_ci(self);
  frame->constructor = false;
  frame->bos = self->bos;
  frame->tos = self->bos + nreg;
  frame->pc = 0;
  frame->inst = candy_proto_get_inst(proto);
  frame->constants = (const candy_wrap_t *)candy_vector_data(candy_proto_get_const(proto));
  self->stack = _stack_reserve(vm, self->ctx, (size_t)frame->tos, self->stack);
  candy_wrap_t *base = self->stack + self->bos;
  uint8_t nlocal = candy_proto_get_nlocal(proto);
  for (uint32_t i = (uint32_t)(self->prev->tos - self->bos); i < nlocal; ++i)
    candy_wrap_set_none(base + i);
  vm->ci = frame;
  self->ci = frame;
  _load_frame(self, frame->inst, frame->constants, nlocal);
  _execute(self);
}

/* The entry closure on the VM stack keeps the entire plan alive. Module
 * initializers are ordinary chunks; the VM never opens or parses files. */
static CANDY_NOINLINE void _initialize_modules(vmll_t *self, const candy_array_t *plan) {
  candy_vm_t *vm = self->vm;
  candy_gc_t *gc = vm->gc;
  candy_excep_t *ctx = self->ctx;
  const candy_array_t *pending = (const candy_array_t *)candy_wrap_get_object(candy_array_get(plan, 0));
  const candy_array_t *aliases = (const candy_array_t *)candy_wrap_get_object(candy_array_get(plan, 1));
  if (!gc->modules)
    gc->modules = candy_table_create(gc, ctx);
  for (size_t i = 0; i < candy_array_size(pending); i += 3) {
    const candy_wrap_t *module = candy_array_get(pending, i);
    const candy_wrap_t *cached = candy_table_get(gc->modules, gc, module);
    if (cached != &CANDY_WRAP_NULL) {
      candy_assert(ctx, gc, candy_wrap_type(cached) == CANDY_TYPE_TABLE,
        CANDY_ERR_VM, "module initialization already in progress");
      continue;
    }
    _vm_push(vm, ctx, module + 2, 1);
    candy_wrap_t loading;
    candy_wrap_set_boolean(&loading, false);
    candy_err_t err = candy_table_set(gc->modules, gc, ctx, module, &loading);
    candy_assert(ctx, gc, err == CANDY_OK, CANDY_ERR_VM, "could not cache module");
    candy_object_t *out = NULL;
    err = candy_vm_call(vm, ctx, 0, 0, self->co, &out);
    if (err != CANDY_OK) {
      candy_table_reset(gc->modules, gc, module);
      candy_excep_throw(ctx, err, out);
    }
    err = candy_table_set(gc->modules, gc, ctx, module, module + 1);
    candy_assert(ctx, gc, err == CANDY_OK, CANDY_ERR_VM, "could not cache module");
  }
  for (size_t i = 0; i < candy_array_size(aliases); i += 2) {
    const candy_wrap_t *binding = candy_array_get(aliases, i);
    candy_err_t err = candy_table_set(candy_gc_global(gc), gc, ctx, binding, binding + 1);
    candy_assert(ctx, gc, err == CANDY_OK, CANDY_ERR_VM, "could not bind module");
  }
  self->stack = (candy_wrap_t *)candy_vector_data(&vm->s);
}

static CANDY_FORCE_INLINE void _call(vmll_t *self) {
  const candy_wrap_t *fn = self->stack + self->bos;
  switch (candy_wrap_type(fn)) {
    case CANDY_TYPE_CFUNC:
      _call_cfunction(self, self->stack + self->bos, 0);
      break;
    case CANDY_TYPE_SCLOS: {
      const candy_proto_t *proto = candy_sclosure_get_proto((candy_sclosure_t *)candy_wrap_get_object(fn));
      const candy_array_t *plan = candy_proto_get_initializers(proto);
      if (plan)
        _initialize_modules(self, plan);
      _call_sclosure(self, proto);
      break;
    }
    default:
      candy_assert(self->ctx, self->vm->gc, false, CANDY_ERR_VM, "function expected");
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

static CANDY_FORCE_INLINE const candy_proto_t *_current_proto(vmll_t *self) {
  const candy_wrap_t *fn = self->stack + self->ci->bos;
  return candy_sclosure_get_proto((candy_sclosure_t *)candy_wrap_get_object(fn));
}

static CANDY_NOINLINE candy_table_t *_module_globals(vmll_t *self) {
  candy_table_t *globals = candy_proto_get_globals(_current_proto(self));
  return globals ? globals : candy_gc_global(self->vm->gc);
}

static CANDY_FORCE_INLINE candy_table_t *_globals(vmll_t *self) {
  candy_gc_t *gc = self->vm->gc;
  return gc->modules ? _module_globals(self) : candy_gc_global(gc);
}

static CANDY_FORCE_INLINE const candy_wrap_t *_read_global(vmll_t *self, candy_table_cache_t *globals, const candy_wrap_t *key) {
  candy_gc_t *gc = self->vm->gc;
  const candy_wrap_t *value = candy_table_get_cached(globals, gc, key);
  if (value == &CANDY_WRAP_NULL && globals->table != candy_gc_global(gc))
    return candy_table_get(candy_gc_global(gc), gc, key);
  return value;
}

static CANDY_FORCE_INLINE void _op_loadg(vmll_t *self, const candy_inst_t *it, candy_wrap_t *base, const candy_wrap_t *k, candy_table_cache_t *globals) {
  const candy_wrap_t *key = k + it->iabx.b;
  const candy_wrap_t *val = _read_global(self, globals, key);
  base[it->iabx.a] = *val;
}

static CANDY_FORCE_INLINE void _op_setg(vmll_t *self, const candy_inst_t *it, candy_wrap_t *base, const candy_wrap_t *k, candy_table_cache_t *globals) {
  const candy_wrap_t *key = k + it->iabx.b;
  const candy_wrap_t *val = base + it->iabx.a;
  candy_err_t err = candy_table_update_cached(globals, self->vm->gc, self->ctx, key, NULL, NULL, val);
  candy_assert(self->ctx, self->vm->gc, err == CANDY_OK, CANDY_ERR_VM, "set global failed");
}

/* Bind the captures of a lambda proto: locals by reference share the frame
 * slot, by value take a snapshot, and upvalues are shared or copied. Kept out
 * of line: only lambda creation pays for it. */
static CANDY_NOINLINE candy_sclosure_t *_closure_bind(vmll_t *self, const candy_proto_t *proto, candy_wrap_t *base) {
  candy_gc_t *gc = self->vm->gc;
  uint8_t nupvals = candy_proto_get_nupvals(proto);
  candy_sclosure_t *closure = candy_sclosure_create(gc, self->ctx, (candy_proto_t *)proto, nupvals);
  const candy_sclosure_t *parent = (const candy_sclosure_t *)candy_wrap_get_object(base);
  for (uint8_t i = 0; i < nupvals; ++i) {
    uint8_t kind = 0;
    uint8_t index = 0;
    candy_proto_get_upval(proto, i, &kind, &index);
    candy_upvalue_t *upval = NULL;
    switch (kind) {
      /* parent local, by reference: share the frame slot */
      case 0: upval = candy_upvalue_open(&self->vm->open, gc, self->ctx, &self->vm->s, self->ci->bos + index); break;
      /* parent upvalue, by reference: share the cell */
      case 1: upval = candy_sclosure_get_upval((candy_sclosure_t *)parent, index); break;
      /* parent local, by value: snapshot the current slot */
      case 2: upval = candy_upvalue_create_copy(gc, self->ctx, base + index); break;
      /* parent upvalue, by value: snapshot the upvalue's current value */
      default: upval = candy_upvalue_create_copy(gc, self->ctx,
        candy_upvalue_view(candy_sclosure_get_upval((candy_sclosure_t *)parent, index))); break;
    }
    candy_sclosure_set_upval(closure, i, upval);
  }
  return closure;
}

/* Create a closure for the lambda proto in `it->iabx.b`. */
static CANDY_FORCE_INLINE void _op_closure(vmll_t *self, const candy_inst_t *it, candy_wrap_t *base, const candy_wrap_t *k) {
  const candy_proto_t *proto = (const candy_proto_t *)candy_wrap_get_object(k + it->iabx.b);
  candy_wrap_set_object(base + it->iabx.a, (candy_object_t *)_closure_bind(self, proto, base));
  _vmll_gc(self);
}

static CANDY_NOINLINE void _op_getupval(vmll_t *self, const candy_inst_t *it, candy_wrap_t *base) {
  (void)self;
  candy_sclosure_t *closure = (candy_sclosure_t *)candy_wrap_get_object(base);
  base[it->iabx.a] = *candy_upvalue_view(candy_sclosure_get_upval(closure, (uint8_t)it->iabx.b));
}

static CANDY_NOINLINE void _op_setupval(vmll_t *self, const candy_inst_t *it, candy_wrap_t *base) {
  (void)self;
  candy_sclosure_t *closure = (candy_sclosure_t *)candy_wrap_get_object(base);
  *candy_upvalue_view(candy_sclosure_get_upval(closure, (uint8_t)it->iabx.b)) = base[it->iabx.a];
}

/* Keep allocation and the GC check out of the dispatch loop. */
static CANDY_NOINLINE void _op_newcontainer(vmll_t *self, const candy_inst_t *it, candy_wrap_t *base) {
  candy_gc_t *gc = self->vm->gc;
  candy_excep_t *ctx = self->ctx;
  candy_object_t *object = it->iabx.b
    ? (candy_object_t *)candy_array_create(gc, ctx, CANDY_TYPE_WRAP)
    : (candy_object_t *)candy_table_create(gc, ctx);
  candy_wrap_set_object(base + it->iabx.a, object);
  _vmll_gc(self);
}

static CANDY_NOINLINE const candy_wrap_t *_class_lookup(vmll_t *self, candy_table_t *table,
    const candy_wrap_t *key, const candy_wrap_t *val) {
  const char class_key[] = "__class__";
  const candy_array_t *name = (const candy_array_t *)candy_wrap_get_object(key);
  if (candy_array_size(name) != sizeof(class_key) - 1 ||
      memcmp(candy_array_data(name), class_key, sizeof(class_key) - 1) != 0) {
    candy_wrap_t meta_key;
    candy_wrap_set_object(&meta_key, (const candy_object_t *)candy_array_create_const(self->vm->gc, self->ctx, CANDY_TYPE_CHAR, class_key, sizeof(class_key) - 1));
    const candy_wrap_t *klass = candy_table_get(table, self->vm->gc, &meta_key);
    if (candy_wrap_type(klass) == CANDY_TYPE_TABLE)
      val = candy_table_get((candy_table_t *)candy_wrap_get_object(klass), self->vm->gc, key);
  }
  return val;
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
    candy_table_t *table = (candy_table_t *)candy_wrap_get_object(container);
    const candy_wrap_t *val = candy_table_get(table, self->vm->gc, key);
    if (candy_wrap_type(val) == CANDY_TYPE_NULL && candy_wrap_type(key) == CANDY_TYPE_CHAR) {
      val = _class_lookup(self, table, key, val);
    }
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
  }
}

static CANDY_FORCE_INLINE void _add_integer(candy_wrap_t *dst, const candy_wrap_t *lhs, candy_integer_t rhs) {
  if (CANDY_LIKELY(candy_wrap_type(lhs) == CANDY_TYPE_INTEGER))
    candy_wrap_set_integer(dst, candy_wrap_get_integer(lhs) + rhs);
  else
    candy_wrap_set_float(dst, candy_wrap_to_float(lhs) + (candy_float_t)rhs);
}

static CANDY_FORCE_INLINE void _add(candy_wrap_t *dst, const candy_wrap_t *lhs, const candy_wrap_t *rhs) {
  if (CANDY_LIKELY(candy_wrap_type(rhs) == CANDY_TYPE_INTEGER))
    _add_integer(dst, lhs, candy_wrap_get_integer(rhs));
  else
    _arith_slow(dst, lhs, rhs, CANDY_ARITH_ADD);
}

/* The opcode selects the right operand's bank; the operation is the same. */
static CANDY_FORCE_INLINE void _op_add(const candy_inst_t *it, candy_wrap_t *base, const candy_wrap_t *rhs_bank) {
  _add(base + it->iabc.a, base + it->iabc.b, rhs_bank + it->iabc.c);
}

static CANDY_FORCE_INLINE void _add_global(vmll_t *self, candy_table_cache_t *globals, const candy_wrap_t *key, const candy_wrap_t *rhs) {
  candy_gc_t *gc = self->vm->gc;
  candy_err_t err = candy_table_update_cached(globals, gc, self->ctx, key, candy_gc_global(gc), _add, rhs);
  candy_assert(self->ctx, gc, err == CANDY_OK, CANDY_ERR_VM, "set global failed");
}

static CANDY_FORCE_INLINE void _op_addg(vmll_t *self, const candy_inst_t *it, const candy_wrap_t *k, const candy_wrap_t *rhs_bank, candy_table_cache_t *globals) {
  _add_global(self, globals, k + it->iabc.b, rhs_bank + it->iabc.c);
}

static CANDY_FORCE_INLINE void _op_sub(const candy_inst_t *it, candy_wrap_t *base, const candy_wrap_t *rhs_bank) {
  candy_wrap_t *dst = base + it->iabc.a;
  const candy_wrap_t *lhs = base + it->iabc.b;
  const candy_wrap_t *rhs = rhs_bank + it->iabc.c;
  if (CANDY_LIKELY(_both_integer(lhs, rhs)))
    candy_wrap_set_integer(dst, candy_wrap_get_integer(lhs) - candy_wrap_get_integer(rhs));
  else
    _arith_slow(dst, lhs, rhs, CANDY_ARITH_SUB);
}

static CANDY_FORCE_INLINE void _op_mul(const candy_inst_t *it, candy_wrap_t *base, const candy_wrap_t *rhs_bank) {
  candy_wrap_t *dst = base + it->iabc.a;
  const candy_wrap_t *lhs = base + it->iabc.b;
  const candy_wrap_t *rhs = rhs_bank + it->iabc.c;
  if (CANDY_LIKELY(_both_integer(lhs, rhs)))
    candy_wrap_set_integer(dst, candy_wrap_get_integer(lhs) * candy_wrap_get_integer(rhs));
  else
    _arith_slow(dst, lhs, rhs, CANDY_ARITH_MUL);
}

static CANDY_FORCE_INLINE void _op_div(const candy_inst_t *it, candy_wrap_t *base, const candy_wrap_t *rhs_bank) {
  candy_wrap_t *dst = base + it->iabc.a;
  const candy_wrap_t *lhs = base + it->iabc.b;
  const candy_wrap_t *rhs = rhs_bank + it->iabc.c;
  if (CANDY_LIKELY(_both_integer(lhs, rhs)))
    candy_wrap_set_integer(dst, candy_wrap_get_integer(lhs) / candy_wrap_get_integer(rhs));
  else
    _arith_slow(dst, lhs, rhs, CANDY_ARITH_DIV);
}

static CANDY_FORCE_INLINE void _op_mod(const candy_inst_t *it, candy_wrap_t *base, const candy_wrap_t *rhs_bank) {
  candy_wrap_t *dst = base + it->iabc.a;
  const candy_wrap_t *lhs = base + it->iabc.b;
  const candy_wrap_t *rhs = rhs_bank + it->iabc.c;
  if (CANDY_LIKELY(_both_integer(lhs, rhs)))
    candy_wrap_set_integer(dst, candy_wrap_get_integer(lhs) % candy_wrap_get_integer(rhs));
  else
    _arith_slow(dst, lhs, rhs, CANDY_ARITH_MOD);
}

static CANDY_FORCE_INLINE bool _less_than(const candy_wrap_t *lhs, const candy_wrap_t *rhs) {
  if (CANDY_LIKELY(_both_integer(lhs, rhs)))
    return candy_wrap_get_integer(lhs) < candy_wrap_get_integer(rhs);
  return candy_wrap_to_float(lhs) < candy_wrap_to_float(rhs);
}

static CANDY_FORCE_INLINE bool _less_equal(const candy_wrap_t *lhs, const candy_wrap_t *rhs) {
  if (CANDY_LIKELY(_both_integer(lhs, rhs)))
    return candy_wrap_get_integer(lhs) <= candy_wrap_get_integer(rhs);
  return candy_wrap_to_float(lhs) <= candy_wrap_to_float(rhs);
}

static CANDY_FORCE_INLINE void _op_le(const candy_inst_t *it, candy_wrap_t *base, const candy_wrap_t *rhs_bank) {
  candy_wrap_set_boolean(base + it->iabc.a, _less_equal(base + it->iabc.b, rhs_bank + it->iabc.c));
}

static CANDY_FORCE_INLINE void _op_lt(const candy_inst_t *it, candy_wrap_t *base, const candy_wrap_t *rhs_bank) {
  candy_wrap_set_boolean(base + it->iabc.a, _less_than(base + it->iabc.b, rhs_bank + it->iabc.c));
}

static CANDY_FORCE_INLINE void _op_eq(const candy_inst_t *it, candy_wrap_t *base, const candy_wrap_t *rhs_bank) {
  candy_wrap_set_boolean(base + it->iabc.a, candy_wrap_equal(base + it->iabc.b, rhs_bank + it->iabc.c));
}

static CANDY_FORCE_INLINE void _op_neq(const candy_inst_t *it, candy_wrap_t *base, const candy_wrap_t *rhs_bank) {
  candy_wrap_set_boolean(base + it->iabc.a, !candy_wrap_equal(base + it->iabc.b, rhs_bank + it->iabc.c));
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

static CANDY_NOINLINE bool _call_constructor(vmll_t *self, candy_wrap_t *base, size_t *pc, uint32_t a, uint32_t b) {
  candy_table_t *klass = (candy_table_t *)candy_wrap_get_object(base + a);
  candy_wrap_t key;
  candy_array_t *name = candy_array_create_const(self->vm->gc, self->ctx, CANDY_TYPE_CHAR, "__init__", 8);
  candy_wrap_set_object(&key, (candy_object_t *)name);
  const candy_wrap_t *init = candy_table_get(klass, self->vm->gc, &key);
  candy_assert(self->ctx, self->vm->gc, candy_wrap_type(init) == CANDY_TYPE_SCLOS, CANDY_ERR_VM, "class constructor expected");
  candy_wrap_t instance;
  candy_table_t *object = candy_table_create(self->vm->gc, self->ctx);
  candy_wrap_set_object(&instance, (candy_object_t *)object);
  candy_wrap_t class_key;
  candy_array_t *class_name = candy_array_create_const(self->vm->gc, self->ctx, CANDY_TYPE_CHAR, "__class__", 9);
  candy_wrap_set_object(&class_key, (candy_object_t *)class_name);
  candy_table_set(object, self->vm->gc, self->ctx, &class_key, base + a);
  memmove(base + a + 2, base + a + 1, b * sizeof(candy_wrap_t));
  base[a] = *init;
  base[a + 1] = instance;
  if (candy_wrap_get_object(base + a) == candy_wrap_get_object(base))
    _frame_push_self(self, *pc, a, b + 1);
  else
    _frame_push(self, *pc, a, b + 1);
  self->vm->ci->constructor = true;
  self->vm->ci->instance = instance;
  *pc = 0;
  return true;
}

static CANDY_NOINLINE bool _call_other(vmll_t *self, candy_wrap_t *base, size_t *pc,
    uint32_t a, uint32_t b, uint32_t c, candy_types_t type) {
  if (type == CANDY_TYPE_TABLE)
    return _call_constructor(self, base, pc, a, b);
  candy_assert(self->ctx, self->vm->gc, type == CANDY_TYPE_CFUNC, CANDY_ERR_VM, "function expected");
  int res = _call_cfunction(self, base, a);
  candy_wrap_t *current = self->stack + self->ci->bos;
  for (uint32_t i = (uint32_t)res; i < c; ++i)
    current[a + i] = CANDY_WRAP_NULL;
  _vmll_gc(self);
  return true;
}

static CANDY_FORCE_INLINE bool _call_at(vmll_t *self, candy_wrap_t *base, size_t *pc, uint32_t a, uint32_t b, uint32_t c) {
  candy_types_t type = candy_wrap_type(base + a);
  if (CANDY_UNLIKELY(type != CANDY_TYPE_SCLOS)) {
    size_t next_pc = *pc;
    bool reload = _call_other(self, base, &next_pc, a, b, c, type);
    *pc = next_pc;
    return reload;
  }
  if (candy_wrap_get_object(base + a) == candy_wrap_get_object(base))
    _frame_push_self(self, *pc, a, b);
  else
    _frame_push(self, *pc, a, b);
  *pc = 0;
  return true;
}

static CANDY_FORCE_INLINE bool _op_call(vmll_t *self, const candy_inst_t *it, candy_wrap_t *base, size_t *pc) {
  return _call_at(self, base, pc, it->iabc.a, it->iabc.b, it->iabc.c);
}

static CANDY_NOINLINE bool _op_callmethod(vmll_t *self, const candy_inst_t *it, candy_wrap_t *base, size_t *pc) {
  uint32_t a = it->iabc.a;
  uint32_t b = it->iabc.b;
  const candy_wrap_t *receiver = base + a + 1;
  if (candy_wrap_type(receiver) == CANDY_TYPE_TABLE) {
    candy_gc_t *gc = self->vm->gc;
    candy_table_t *table = (candy_table_t *)candy_wrap_get_object(receiver);
    candy_wrap_t key;
    candy_array_t *name = candy_array_create_const(gc, self->ctx, CANDY_TYPE_CHAR, "__class__", 9);
    candy_wrap_set_object(&key, (candy_object_t *)name);
    if (candy_wrap_type(candy_table_get(table, gc, &key)) == CANDY_TYPE_TABLE)
      return _call_at(self, base, pc, a, b + 1, it->iabc.c);
  }
  /* An ordinary table field call passes only the user-supplied arguments. */
  memmove(base + a + 1, base + a + 2, b * sizeof(candy_wrap_t));
  return _call_at(self, base, pc, a, b, it->iabc.c);
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
  if (self->vm->open != NULL)
    candy_upvalue_close_all(&self->vm->open, self->vm->ci->bos);
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
    self->stack = _stack_reserve(self->vm, self->ctx, (size_t)self->ci->bos + nreg, self->stack);
    base = self->stack + self->ci->bos;
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
  candy_assert(self->ctx, self->vm->gc, candy_wrap_type(fn) == CANDY_TYPE_CFUNC, CANDY_ERR_VM, "function expected");
  if (a != 0)
    memmove(base, base + a, (b + 1) * sizeof(candy_wrap_t));
  int res = _call_cfunction(self, self->stack + self->ci->bos, 0);
  if (res < 1)
    self->stack[self->ci->bos] = CANDY_WRAP_NULL;
  _vmll_gc(self);
  return _frame_return(self, pc);
}

static CANDY_FORCE_INLINE void _op_tailcallself(vmll_t *self, const candy_inst_t *it, candy_wrap_t *base, size_t *pc) {
  if (self->vm->open != NULL)
    candy_upvalue_close_all(&self->vm->open, self->vm->ci->bos);
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

static CANDY_HOT CANDY_CODE_ALIGN(64) void _execute(vmll_t *self) {
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

  const candy_vector_t *code = NULL;
  const candy_inst_t *instructions;
  size_t inst_count;
  const candy_wrap_t *constants;
  candy_table_cache_t globals = {0};
  candy_wrap_t *base;
  size_t pc = 0;
reload:
  /* Instructions, constants and the global environment belong to the prototype. */
  if (code != self->inst) {
    code = self->inst;
    instructions = candy_vector_data(code);
    inst_count = candy_vector_size(code);
    constants = self->constants;
    candy_table_cache_bind(&globals, _globals(self));
  }
  base = self->stack + self->ci->bos;
  while (pc < inst_count) {
    const candy_inst_t *it = instructions + pc++;
    vmdispatch(it->op) {
      #define CANDY_OPCODE_CASE
      #include "core/candy_opcode.list"
    }
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
  if (vm->open != NULL)
    candy_upvalue_close_all(&vm->open, self->bos);
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
  self->open = NULL;
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
