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
extern "C"{
#endif /* __cplusplus */

#include "src/candy_wrap.h"
#include "src/candy_types.h"

typedef enum candy_opcodes {
  #define CANDY_OP_ENUM
  #include "src/candy_opcode.list"
} candy_opcodes_t;

struct candy_block {
  candy_block_t *prev;
  candy_wrap_t pool;
  candy_wrap_t opcode;
};

candy_block_t *candy_block_create(candy_block_t *prev);
int candy_block_delete(candy_block_t **self);

candy_wrap_t *candy_block_add_const(candy_block_t *self);
candy_opcode_t *candy_block_add_op(candy_block_t *self);

#ifdef __cplusplus
}
#endif /* __cplusplus */
#endif /* CANDY_SRC_BLOCK_H */
