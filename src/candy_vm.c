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
#include "src/candy_wrap.h"
#include "src/candy_table.h"
#include "src/candy_io.h"
#include "src/candy_lib.h"
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#define CANDY_OP_SIZE 6

#define vm_assert(_condition, _format, ...) candy_assert(_condition, vm, _format, ##__VA_ARGS__)

typedef enum candy_opcode {
  #define CANDY_OP_ENUM
  #include "src/candy_opcode.list"
} candy_opcode_t;

typedef union candy_opmode {
  struct {
    uint32_t op : CANDY_OP_SIZE;
    uint32_t    :            24;
  };
  struct {
    uint32_t    : CANDY_OP_SIZE;
    uint32_t  a :             8;
    uint32_t  b :             9;
    uint32_t  c :             9;
  } iABC;
  struct {
    uint32_t    : CANDY_OP_SIZE;
    uint32_t  a :             8;
    uint32_t  b :            18;
  } iABx;
} candy_opmode_t;

struct candy_vm {
  candy_state_t *sta;
  candy_wrap_t *base;
  size_t size;
  candy_table_t *glb;
};

void _push(candy_vm_t *self, const candy_wrap_t *wrap) {
  candy_wrap_t *dst = (candy_wrap_t *)expand(self->base, sizeof(candy_wrap_t), self->size, self->size + 1);
  if (self->base != dst) {
    free(self->base);
    self->base = dst;
  }
  self->base[self->size++] = *wrap;
}

const candy_wrap_t *_pop(candy_vm_t *self) {
  return self->size ? &self->base[--self->size] : &null;
}

candy_vm_t *candy_vm_create(candy_state_t *sta) {
  candy_vm_t *self = (candy_vm_t *)calloc(1, sizeof(struct candy_vm));
  self->sta = sta;
  self->base = calloc(CANDY_DEFAULT_STACK_SIZE, sizeof(candy_wrap_t));
  self->glb = candy_table_create();
  return self;
}

int candy_vm_delete(candy_vm_t **self) {
  free((*self)->base);
  (*self)->base = NULL;
  candy_table_delete(&(*self)->glb);
  free(*self);
  *self = NULL;
  return 0;
}

int candy_vm_dump_global(candy_vm_t *self) {
  return candy_table_dump(self->glb);
}

int candy_vm_builtin(candy_vm_t *self, candy_regist_t list[], size_t size) {
  for (size_t idx = 0; idx < size; ++idx) {
    candy_wrap_t key, val;
    candy_wrap_init(&key);
    candy_wrap_init(&val);
    candy_wrap_set_string(&key, list[idx].name, strlen(list[idx].name) + 1);
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
