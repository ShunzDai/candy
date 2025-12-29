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
#ifndef CANDY_CORE_PROTO_H
#define CANDY_CORE_PROTO_H
#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#include "core/candy_priv.h"

typedef enum candy_opcodes {
  #define CANDY_OPCODE_ENUM
  #include "core/candy_opcode.list"
} candy_opcodes_t;

union candy_inst {
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
};

typedef struct candy_proto_config candy_proto_config_t;

struct candy_proto_config {
  const candy_vector_t *inst;
};

candy_proto_t *candy_proto_create(candy_gc_t *gc, candy_excep_t *ctx);

candy_err_t candy_proto_handler(candy_proto_t *self, candy_gc_t *gc, candy_events_t evt, void *arg);

size_t candy_proto_add_const(candy_proto_t *self, candy_gc_t *gc, candy_excep_t *ctx, const candy_wrap_t *wrap);

size_t candy_proto_add_inst(candy_proto_t *self, candy_gc_t *gc, candy_excep_t *ctx, candy_inst_t inst);

const candy_vector_t *candy_proto_get_const(const candy_proto_t *self);

const candy_vector_t *candy_proto_get_inst(const candy_proto_t *self);

#ifdef __cplusplus
}
#endif /* __cplusplus */
#endif /* CANDY_CORE_PROTO_H */
