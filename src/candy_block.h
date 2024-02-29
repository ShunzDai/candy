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
#ifndef CANDY_SRC_BLOCK_H
#define CANDY_SRC_BLOCK_H
#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#include "src/candy_wrap.h"
#include "src/candy_types.h"

typedef enum candy_opcodes {
  #define CANDY_OP_ENUM
  #include "src/candy_opcode.list"
} candy_opcodes_t;

typedef union candy_instruc {
  struct {
    uint32_t op :  6;
    uint32_t    : 26;
  };
  struct {
    uint32_t op :  6;
    uint32_t  a : 26;
  } iax;
  struct {
    uint32_t op :  6;
    uint32_t  a :  8;
    uint32_t  b : 18;
  } iabx;
  struct {
    uint32_t op :  6;
    uint32_t  a :  8;
    uint32_t  b :  9;
    uint32_t  c :  9;
  } iabc;
} candy_instruc_t;

struct candy_block {
  candy_wrap_t pool;
  candy_wrap_t ins;
};

candy_block_t *candy_block_create(void);
int candy_block_delete(candy_block_t **self);

int candy_block_add_const(candy_block_t *self, const candy_wrap_t *wrap);

static inline void candy_block_add_iax(candy_block_t *self, candy_opcodes_t op, uint32_t a) {
  int candy_block_add_instruc(candy_block_t *self, candy_instruc_t *ins);
  candy_instruc_t ins = {
    .iax = {
      .op = (uint32_t)op,
      .a = a,
    },
  };
  candy_block_add_instruc(self, &ins);
}

static inline void candy_block_add_iabx(candy_block_t *self, candy_opcodes_t op, uint32_t a, uint32_t b) {
  int candy_block_add_instruc(candy_block_t *self, candy_instruc_t *ins);
  candy_instruc_t ins = {
    .iabx = {
      .op = (uint32_t)op,
      .a = a,
      .b = b,
    },
  };
  candy_block_add_instruc(self, &ins);
}

static inline void candy_block_add_iabc(candy_block_t *self, candy_opcodes_t op, uint32_t a, uint32_t b, uint32_t c) {
  int candy_block_add_instruc(candy_block_t *self, candy_instruc_t *ins);
  candy_instruc_t ins = {
    .iabc = {
      .op = (uint32_t)op,
      .a = a,
      .b = b,
      .c = c,
    },
  };
  candy_block_add_instruc(self, &ins);
}

#ifdef __cplusplus
}
#endif /* __cplusplus */
#endif /* CANDY_SRC_BLOCK_H */
