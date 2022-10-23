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
#include "candy_vm.h"
#include "candy_wrap.h"
#include "candy_queue.h"
#include "candy_parser.h"
#include <stdlib.h>

#define CANDY_OP_SIZE 6

typedef enum candy_opcode {
  #define CANDY_OP_ENUM
  #include "candy_opcode.list"
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

struct candy_opview {
  const uint32_t size;
  const union candy_opmode mode[];
};

struct candy_stack {
  struct candy_stack *next;
  struct candy_queue *args;
};

struct candy_vm {
  struct candy_queue *stack;
  struct candy_opview *opview;
  uint32_t opcurr;
};

static struct candy_stack *_stack_create() {
  struct candy_stack *stack = (struct candy_stack *)malloc(sizeof(struct candy_stack));
  stack->next = NULL;
  stack->args = candy_queue_create();
  return stack;
}

int _stack_delete(struct candy_stack **stack) {
  candy_queue_delete(&(*stack)->args, (candy_destroy_t)candy_wrap_delete);
  free(*stack);
  *stack = NULL;
  return 0;
}

static inline int _stack_push(struct candy_queue *stack) {
  return candy_enqueue(stack, 0, (struct candy_node *)_stack_create());
}

static inline int _stack_pop(struct candy_queue *stack) {
  return candy_dequeue(stack, 0, (candy_destroy_t)_stack_delete);
}

struct candy_vm *candy_vm_create() {
  struct candy_vm *vm = (struct candy_vm *)malloc(sizeof(struct candy_vm));
  vm->stack = candy_queue_create();
  vm->opview = NULL;
  vm->opcurr = 0;
  return vm;
}

int candy_vm_delete(struct candy_vm **vm) {
  free(*vm);
  *vm = NULL;
  return 0;
}

int candy_vm_load(struct candy_vm *vm, const uint8_t binary[]) {
  vm->opview = (struct candy_opview *)binary;
  return 0;
}

int candy_vm_execute(struct candy_vm *vm) {
  // const struct candy_stack *base = (struct candy_stack *)*candy_queue_pointer(vm->stack, 0);
  while (vm->opcurr < vm->opview->size) {
    switch (vm->opview->mode[vm->opcurr].op) {
      #define CANDY_OP_CASE
      #include "candy_opcode.list"
    }
  }
  return 0;
}
