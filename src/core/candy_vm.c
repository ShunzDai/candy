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
#include "src/struct/candy_wrap.h"
#include "src/core/candy_parser.h"

#define CANDY_OP_SIZE 6

typedef enum candy_opcode {
  #define CANDY_OP_ENUM
  #include "src/core/candy_opcode.list"
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
  struct candy_queue *stack;
};

int candy_vm_load(struct candy_vm *vm, const uint8_t binary[]) {
  return 0;
}

int candy_vm_execute(struct candy_vm *vm) {
  const uint32_t *cursor = (uint32_t *)0;
  for (uint32_t size = *cursor++; size; size--) {

    switch (((candy_opmode_t *)cursor++)->op) {
      #define CANDY_OP_CASE
      #include "src/core/candy_opcode.list"
    }
  }
  return 0;
}
