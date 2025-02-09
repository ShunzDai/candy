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
#include "core/candy_exception.h"
#include "core/candy_gc.h"
#include "core/candy_wrap.h"
#include "core/candy_vector.h"
#include "core/candy_table.h"
#include "core/candy_proto.h"
#include "core/candy_print.h"
#include <string.h>
#include <stdlib.h>

#define vm_assert(_condition, _format, ...) \
candy_assert(&self->ctx, self->gc, _condition, CANDY_ERR_VM, _format, ##__VA_ARGS__)

typedef struct callinfo callinfo_t;
typedef struct vmll vmll_t;

struct callinfo {
  callinfo_t *prev;
  callinfo_t *next;
  candy_wrap_t *tos;
  candy_wrap_t *bos;
  candy_inst_t *pc;
  int nres;
};

struct vmll {
  candy_vm_t *vm;
  callinfo_t *ci;
  candy_state_t *co;
};

static candy_err_t _vmll_init(vmll_t *self, candy_vm_t *vm, callinfo_t *ci, candy_state_t *co) {
  self->vm = vm;
  self->ci = ci;
  self->co = co;
  return CANDY_OK;
}

static candy_err_t _vmll_deinit(vmll_t *self) {
  return CANDY_OK;
}

// static void _op_loadc(candy_vm_t *self, const candy_inst_t *pc) {
//   candy_wrap_t *reg = NULL;
//   reg[pc->iabc.a] = reg[pc->iabc.b];
// }

static void _execute(vmll_t *self, callinfo_t *ci) {
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
  _execute(self, self->ci);
}

candy_err_t candy_vm_init(candy_vm_t *self, candy_gc_t *gc) {
  candy_excep_init(&self->ctx);
  candy_vector_init(&self->s, sizeof(candy_wrap_t));
  self->gc = gc;
  return CANDY_OK;
}

candy_err_t candy_vm_deinit(candy_vm_t *self) {
  candy_vector_deinit(&self->s, candy_gc_memory(self->gc));
  candy_excep_deinit(&self->ctx);
  return CANDY_OK;
}

// candy_err_t candy_vm_fprint(candy_vm_t *self, FILE *out) {
//   return candy_wrap_fprint(candy_vm_pop(self), out, 0, candy_table_fprint);
// }

// candy_err_t candy_vm_regist(candy_vm_t *self, const candy_regist_t list[], int size) {
//   for (int idx = 0; idx < size; ++idx) {
//     candy_wrap_t key = {0}, val = {0};
//     candy_wrap_set_string(&key, list[idx].name, strlen(list[idx].name));
//     candy_wrap_set_cfunc(&val, &list[idx].func, 1);
//     candy_table_set(&self->glb, &key, &val);
//   }
//   return CANDY_OK;
// }

// candy_err_t candy_vm_set_global(candy_vm_t *self, const char name[]) {
//   candy_wrap_t key;
//   candy_wrap_set_string(&key, name, strlen(name));
//   candy_table_set(&self->glb, &key, candy_vm_pop(self));
//   candy_wrap_deinit(&key);
//   return CANDY_OK;
// }

// candy_err_t candy_vm_get_global(candy_vm_t *self, const char name[]) {
//   candy_wrap_t key;
//   candy_wrap_set_string(&key, name, strlen(name));
//   candy_vm_push(self, candy_table_get(&self->glb, &key));
//   candy_wrap_deinit(&key);
//   return CANDY_OK;
// }

// candy_err_t candy_vm_call(candy_vm_t *self, int nargs, int nresults) {
//   (*candy_wrap_get_cfunc(candy_vm_pop(self)))((candy_state_t *)self);
//   return CANDY_OK;
// }

candy_err_t candy_vm_call(candy_vm_t *self, candy_state_t *co) {
  callinfo_t ci;
  vmll_t vmll;
  _vmll_init(&vmll, self, &ci, co);
  candy_err_t err = candy_excep_try(&self->ctx, (candy_excep_cb_t)_call, &vmll, NULL);
  _vmll_deinit(&vmll);
  return err;
}

candy_err_t candy_vm_pop(candy_vm_t *self) {
  size_t size = candy_vector_size(&self->s);
  vm_assert(size, "stack is already empty");
  candy_vector_resize(&self->s, candy_gc_memory(self->gc), &self->ctx, size - 1);
  return CANDY_OK;
}

candy_err_t candy_vm_push(candy_vm_t *self, const candy_wrap_t *wrap) {
  size_t size = candy_vector_size(&self->s);
  vm_assert(size < CANDY_CONFIG_VM_STACK_SIZE, "stack overflow");
  candy_vector_append(&self->s, candy_gc_memory(self->gc), &self->ctx, wrap, 1);
  return CANDY_OK;
}

const candy_wrap_t *candy_vm_view(candy_vm_t *self, int idx) {
  size_t size = candy_vector_size(&self->s);
  if (idx < 0) {
    vm_assert(size >= (size_t)(-idx), "stack index out of bounds");
    return (const candy_wrap_t *)candy_vector_data(&self->s) + size + idx;
  }
  else {
    vm_assert(size > (size_t)idx, "stack index out of bounds");
    return (const candy_wrap_t *)candy_vector_data(&self->s) + idx;
  }
}
