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
#include "src/candy_stack.h"
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#define CANDY_OP_SIZE 6

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
  candy_stack_t *stack;
  candy_table_t *glb;
  void *gc;
  void *err;
  candy_callinfo_t info;
};

candy_vm_t *candy_vm_create(candy_state_t *sta) {
  candy_vm_t *self = (candy_vm_t *)malloc(sizeof(candy_vm_t));
  self->stack = candy_stack_create();
  self->glb = candy_table_create();
  self->sta = sta;
  return self;
}

int candy_vm_delete(candy_vm_t **self) {
  candy_stack_delete(&(*self)->stack);
  candy_table_delete(&(*self)->glb);
  free(*self);
  *self = NULL;
  return 0;
}

int candy_vm_builtin(candy_vm_t *self, const char name[], candy_regist_t list[], size_t size) {
  printf("obj [%s]\n", name);
  for (unsigned idx = 0; idx < size; ++idx) {
    candy_wrap_t key, val;
    candy_wrap_init(&key);
    candy_wrap_init(&val);
    candy_wrap_set_string(&key, list[idx].name, strlen(list[idx].name) + 1);
    candy_wrap_set_builtin(&val, &list[idx].func, 1);
    candy_table_set(self->glb, &key, &val);
  }
  candy_table_print(self->glb);
  return 0;
}

int candy_vm_call(candy_vm_t *self, const char name[], int nargs, int nresults) {
  candy_wrap_t key;
  candy_wrap_set_string(&key, name, strlen(name) + 1);
  const candy_wrap_t *func = candy_table_get(self->glb, &key);
  candy_wrap_deinit(&key);
  if (func->type != CANDY_BUILTIN)
    return -1;
  self->info.func = *candy_wrap_get_builtin(func);
  candy_wrap_set_string(&key, "__entry__", 10);
  const candy_wrap_t *entry = candy_table_get(self->glb, &key);
  candy_wrap_deinit(&key);
  (*candy_wrap_get_builtin(entry->type != CANDY_BUILTIN ? func : entry))(self->sta);
  return 0;
}

candy_callinfo_t *candy_vm_callinfo(candy_vm_t *self) {
  return &self->info;
}

void candy_vm_push_integer(candy_vm_t *self, const candy_integer_t val) {
  candy_stack_push_integer(self->stack, val);
}

void candy_vm_push_float(candy_vm_t *self, const candy_float_t val) {
  candy_stack_push_float(self->stack, val);
}

void candy_vm_push_boolean(candy_vm_t *self, const candy_boolean_t val) {
  candy_stack_push_boolean(self->stack, val);
}

void candy_vm_push_string(candy_vm_t *self, const char val[], size_t size) {
  candy_stack_push_string(self->stack, val, size);
}

candy_integer_t candy_vm_pull_integer(candy_vm_t *self) {
  /** @todo error handle */
  candy_stack_type(self->stack);
  /** @todo if it is lval, push it into gc */
  return candy_stack_pull_integer(self->stack);
}

candy_float_t candy_vm_pull_float(candy_vm_t *self) {
  /** @todo error handle */
  candy_stack_type(self->stack);
  /** @todo if it is lval, push it into gc */
  return candy_stack_pull_float(self->stack);
}

candy_boolean_t candy_vm_pull_boolean(candy_vm_t *self) {
  /** @todo error handle */
  candy_stack_type(self->stack);
  /** @todo if it is lval, push it into gc */
  return candy_stack_pull_boolean(self->stack);
}

const char *candy_vm_pull_string(candy_vm_t *self, size_t *size) {
  /** @todo error handle */
  candy_stack_type(self->stack);
  /** @todo if it is lval, push it into gc */
  return candy_stack_pull_string(self->stack, size);
}

int candy_vm_execute(candy_vm_t *self) {

  while (1) {
    switch (1) {
      #define CANDY_OP_CASE
      #include "src/candy_opcode.list"
    }
  }
  return 0;
}
