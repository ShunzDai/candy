/**
  * Copyright 2022 ShunzDai
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
#include "src/candy_stack.h"
#include <stdlib.h>
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
  candy_stack_t *stack;
  candy_cfunc_t entry;
  candy_state_t *sta;
  void *gc;
  void *err;
};

candy_vm_t *candy_vm_create(candy_state_t *sta) {
  candy_vm_t *self = (candy_vm_t *)malloc(sizeof(candy_vm_t));
  self->stack = candy_stack_create(4);
  self->entry = NULL;
  self->sta = sta;
  return self;
}

int candy_vm_delete(candy_vm_t **self) {
  candy_stack_delete(&(*self)->stack);
  free(*self);
  *self = NULL;
  return 0;
}

void candy_vm_regist_entry(candy_vm_t *self, candy_cfunc_t entry) {
  self->entry = entry;
}

void candy_vm_push_integer(candy_vm_t *self, candy_integer_t val) {
  candy_stack_push_integer(self->stack, &val, 1);
}

void candy_vm_push_float(candy_vm_t *self, candy_float_t val) {
  candy_stack_push_float(self->stack, &val, 1);
}

void candy_vm_push_boolean(candy_vm_t *self, candy_boolean_t val) {
  candy_stack_push_boolean(self->stack, &val, 1);
}

void candy_vm_push_string(candy_vm_t *self, char *val, int size) {
  candy_stack_push_string(self->stack, val, size);
}

candy_integer_t candy_vm_pull_integer(candy_vm_t *self) {
  /** @todo error handle */
  candy_stack_check_type(self->stack, CANDY_INTEGER);
  /** @todo if it is lval, push it into gc */
  return *candy_stack_pull_integer(self->stack, NULL);
}

candy_float_t candy_vm_pull_float(candy_vm_t *self) {
  /** @todo error handle */
  candy_stack_check_type(self->stack, CANDY_FLOAT);
  /** @todo if it is lval, push it into gc */
  return *candy_stack_pull_float(self->stack, NULL);
}

candy_boolean_t candy_vm_pull_boolean(candy_vm_t *self) {
  /** @todo error handle */
  candy_stack_check_type(self->stack, CANDY_BOOLEAN);
  /** @todo if it is lval, push it into gc */
  return *candy_stack_pull_boolean(self->stack, NULL);
}

char *candy_vm_pull_string(candy_vm_t *self, int *size) {
  /** @todo error handle */
  candy_stack_check_type(self->stack, CANDY_STRING);
  /** @todo if it is lval, push it into gc */
  return candy_stack_pull_string(self->stack, size);
}

int candy_vm_load(candy_vm_t *self, const uint8_t binary[]) {

  return 0;
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
