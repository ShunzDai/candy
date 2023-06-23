/**
  * Copyright 2022-2023 ShunzDai
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
#include "src/candy_vm.h"
#include "src/candy_table.h"
#include "src/candy_block.h"
#include "src/candy_lib.h"
#include <string.h>
#include <assert.h>

#define vm_assert(_condition, _format, ...) candy_assert((candy_io_t *)(self), _condition, vm, _format, ##__VA_ARGS__)

static void execute(candy_vm_t *self, candy_block_t *block) {
  for (const candy_instruc_t *ins = (const candy_instruc_t *)candy_wrap_get_uint32(&block->ins); (size_t)(ins - (const candy_instruc_t *)candy_wrap_get_uint32(&block->ins)) < (size_t)block->ins.size; ++ins) {
    switch (ins->op) {
      #define CANDY_OP_CASE
      #include "src/candy_opcode.list"
      default:
      break;
    }
  }
}

int candy_vm_init(candy_vm_t *self) {
  candy_io_init(&self->io);
  candy_wrap_set_pair(&self->glb, NULL, 16);
  candy_wrap_set_wrap(&self->base, NULL, CANDY_DEFAULT_STACK_SIZE);
  return 0;
}

int candy_vm_deinit(candy_vm_t *self) {
  candy_wrap_deinit(&self->base);
  candy_wrap_deinit(&self->glb);
  candy_io_deinit(&self->io);
  return 0;
}

int candy_vm_fprint(candy_vm_t *self, FILE *out) {
  return candy_wrap_fprint(candy_vm_pop(self), out, 0, candy_table_fprint);
}

void candy_vm_push(candy_vm_t *self, const candy_wrap_t *wrap) {
  if (self->top < self->base.size)
    ((candy_wrap_t *)candy_wrap_get_wrap(&self->base))[self->top] = *wrap;
  else
    candy_wrap_append(&self->base, wrap, 1);
  ++self->top;
}

const candy_wrap_t *candy_vm_pop(candy_vm_t *self) {
  return self->top ? &candy_wrap_get_wrap(&self->base)[--self->top] : &null;
}

int candy_vm_regist(candy_vm_t *self, const candy_regist_t list[], size_t size) {
  for (size_t idx = 0; idx < size; ++idx) {
    candy_wrap_t key = {0}, val = {0};
    candy_wrap_set_string(&key, list[idx].name, strlen(list[idx].name));
    candy_wrap_set_cfunc(&val, &list[idx].func, 1);
    candy_table_set(&self->glb, &key, &val);
  }
  return 0;
}

int candy_vm_set_global(candy_vm_t *self, const char name[]) {
  candy_wrap_t key;
  candy_wrap_set_string(&key, name, strlen(name));
  candy_table_set(&self->glb, &key, candy_vm_pop(self));
  candy_wrap_deinit(&key);
  return 0;
}

int candy_vm_get_global(candy_vm_t *self, const char name[]) {
  candy_wrap_t key;
  candy_wrap_set_string(&key, name, strlen(name));
  candy_vm_push(self, candy_table_get(&self->glb, &key));
  candy_wrap_deinit(&key);
  return 0;
}

int candy_vm_call(candy_vm_t *self, int nargs, int nresults) {
  (*candy_wrap_get_cfunc(candy_vm_pop(self)))((candy_state_t *)self);
  return 0;
}

int candy_vm_execute(candy_vm_t *self, candy_block_t *block) {
  return candy_io_try_catch(&self->io, (candy_try_catch_cb_t)execute, self, block);
}
