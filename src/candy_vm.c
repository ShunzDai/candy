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
#include "src/candy_io.h"
#include "src/candy_table.h"
#include "src/candy_block.h"
#include "src/candy_lib.h"
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#define vm_assert(_condition, _format, ...) candy_assert(_condition, vm, _format, ##__VA_ARGS__)

void _push(candy_vm_t *self, const candy_wrap_t *wrap) {
  candy_wrap_append_wrap(&self->base, wrap, 1);
}

const candy_wrap_t *_pop(candy_vm_t *self) {
  return self->base.size ? &candy_wrap_get_wrap(&self->base)[--self->base.size] : &null;
}

int candy_vm_init(candy_vm_t *self, candy_state_t *sta) {
  self->sta = sta;
  self->base.type = CANDY_WRAP;
  self->glb = candy_table_create();
  return 0;
}

int candy_vm_deinit(candy_vm_t *self) {
  candy_wrap_deinit(&self->base);
  candy_table_delete(&self->glb);
  return 0;
}

int candy_vm_dump_global(candy_vm_t *self) {
  return candy_table_dump(self->glb);
}

int candy_vm_builtin(candy_vm_t *self, candy_regist_t list[], size_t size) {
  for (size_t idx = 0; idx < size; ++idx) {
    candy_wrap_t key = {0}, val = {0};
    candy_wrap_set_string(&key, list[idx].name, strlen(list[idx].name));
    candy_wrap_set_builtin(&val, &list[idx].func, 1);
    candy_table_set(self->glb, &key, &val);
  }
  return 0;
}

int candy_vm_set_global(candy_vm_t *self, const char name[]) {
  candy_wrap_t key;
  candy_wrap_set_string(&key, name, strlen(name));
  candy_table_set(self->glb, &key, _pop(self));
  candy_wrap_deinit(&key);
  return 0;
}

int candy_vm_get_global(candy_vm_t *self, const char name[]) {
  candy_wrap_t key;
  candy_wrap_set_string(&key, name, strlen(name));
  _push(self, candy_table_get(self->glb, &key));
  candy_wrap_deinit(&key);
  return 0;
}

int candy_vm_call(candy_vm_t *self, int nargs, int nresults) {
  (*candy_wrap_get_builtin(_pop(self)))(self->sta);
  return 0;
}

int candy_vm_execute(candy_vm_t *self, candy_block_t *block) {
  for (const candy_instruc_t *ins = candy_wrap_get_instruc(&block->ins); (size_t)(ins - candy_wrap_get_instruc(&block->ins)) < (size_t)block->ins.size; ++ins) {
    switch (ins->op) {
      #define CANDY_OP_CASE
      #include "src/candy_opcode.list"
      default:
      break;
    }
  }
  return 0;
}

void candy_vm_push_integer(candy_vm_t *self, const candy_integer_t val[], size_t size) {
  candy_wrap_t wrap;
  candy_wrap_set_integer(&wrap, val, size);
  _push(self, &wrap);
}

void candy_vm_push_float(candy_vm_t *self, const candy_float_t val[], size_t size) {
  candy_wrap_t wrap;
  candy_wrap_set_float(&wrap, val, size);
  _push(self, &wrap);
}

void candy_vm_push_boolean(candy_vm_t *self, const candy_boolean_t val[], size_t size) {
  candy_wrap_t wrap;
  candy_wrap_set_boolean(&wrap, val, size);
  _push(self, &wrap);
}

void candy_vm_push_string(candy_vm_t *self, const char val[], size_t size) {
  candy_wrap_t wrap;
  candy_wrap_set_string(&wrap, val, size);
  _push(self, &wrap);
}

void candy_vm_push_ud(candy_vm_t *self, const void *val[], size_t size) {
  candy_wrap_t wrap;
  candy_wrap_set_ud(&wrap, val, size);
  _push(self, &wrap);
}

void candy_vm_push_builtin(candy_vm_t *self, const candy_builtin_t val[], size_t size) {
  candy_wrap_t wrap;
  candy_wrap_set_builtin(&wrap, val, size);
  _push(self, &wrap);
}

const candy_integer_t *candy_vm_pull_integer(candy_vm_t *self, size_t *size) {
  const candy_wrap_t *wrap = _pop(self);
  if (size)
    *size = wrap->size;
  return candy_wrap_get_integer(wrap);
}

const candy_float_t *candy_vm_pull_float(candy_vm_t *self, size_t *size) {
  const candy_wrap_t *wrap = _pop(self);
  if (size)
    *size = wrap->size;
  return candy_wrap_get_float(wrap);
}

const candy_boolean_t *candy_vm_pull_boolean(candy_vm_t *self, size_t *size) {
  const candy_wrap_t *wrap = _pop(self);
  if (size)
    *size = wrap->size;
  return candy_wrap_get_boolean(wrap);
}

const char *candy_vm_pull_string(candy_vm_t *self, size_t *size) {
  const candy_wrap_t *wrap = _pop(self);
  if (size)
    *size = wrap->size;
  return candy_wrap_get_string(wrap);
}

const void **candy_vm_pull_ud(candy_vm_t *self, size_t *size) {
  const candy_wrap_t *wrap = _pop(self);
  if (size)
    *size = wrap->size;
  return candy_wrap_get_ud(wrap);
}

const candy_builtin_t *candy_vm_pull_builtin(candy_vm_t *self, size_t *size) {
  const candy_wrap_t *wrap = _pop(self);
  if (size)
    *size = wrap->size;
  return candy_wrap_get_builtin(wrap);
}
