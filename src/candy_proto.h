/**
  * Copyright 2022-2024 ShunzDai
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
#ifndef CANDY_SRC_PROTO_H
#define CANDY_SRC_PROTO_H
#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#include "src/candy_types.h"

typedef enum candy_opcodes {
  #define CANDY_OP_ENUM
  #include "src/candy_opcode.list"
} candy_opcodes_t;

typedef union candy_inst {
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
} candy_inst_t;

typedef struct candy_proto_config candy_proto_config_t;

struct candy_proto_config {
  const candy_vector_t *inst;
};

candy_proto_t *candy_proto_create(candy_gc_t *gc);
int candy_proto_delete(candy_proto_t *self, candy_gc_t *gc);

static inline void candy_proto_add_iax(candy_proto_t *self, candy_opcodes_t op, uint32_t a) {
  int candy_proto_add_inst(candy_proto_t *self, candy_inst_t inst);
  candy_proto_add_inst(self, (candy_inst_t) {
    .iax = {
      .op = (uint32_t)op,
      .a = a,
    },
  });
}

static inline void candy_proto_add_iabx(candy_proto_t *self, candy_opcodes_t op, uint32_t a, uint32_t b) {
  int candy_proto_add_inst(candy_proto_t *self, candy_inst_t inst);
  candy_proto_add_inst(self, (candy_inst_t) {
    .iabx = {
      .op = (uint32_t)op,
      .a = a,
      .b = b,
    },
  });
}

static inline void candy_proto_add_iabc(candy_proto_t *self, candy_opcodes_t op, uint32_t a, uint32_t b, uint32_t c) {
  int candy_proto_add_inst(candy_proto_t *self, candy_inst_t inst);
  candy_proto_add_inst(self, (candy_inst_t) {
    .iabc = {
      .op = (uint32_t)op,
      .a = a,
      .b = b,
      .c = c,
    },
  });
}

candy_inst_t *candy_proto_get_inst(candy_proto_t *self);

#ifdef __cplusplus
}
#endif /* __cplusplus */
#endif /* CANDY_SRC_PROTO_H */
