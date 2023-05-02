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
#include "src/candy_block.h"
#include <stdlib.h>

candy_block_t *candy_block_create(candy_block_t *prev) {
  candy_block_t *self = (candy_block_t *)calloc(1, sizeof(struct candy_block));
  self->prev = prev;
  return self;
}

int candy_block_delete(candy_block_t **self) {
  candy_wrap_deinit(&(*self)->opcode);
  candy_wrap_deinit(&(*self)->pool);
  free(*self);
  *self = NULL;
  return 0;
}

candy_wrap_t *candy_block_add_const(candy_block_t *self) {
  self->pool.type == CANDY_NULL ? candy_wrap_set_wrap(&self->pool, NULL, 1) : candy_wrap_append_wrap(&self->pool, NULL, 1);
  return (candy_wrap_t *)&candy_wrap_get_wrap(&self->pool)[self->pool.size - 1];
}

candy_opcode_t *candy_block_add_op(candy_block_t *self) {
  self->opcode.type == CANDY_NULL ? candy_wrap_set_opcode(&self->opcode, NULL, 1) : candy_wrap_append_opcode(&self->opcode, NULL, 1);
  return (candy_opcode_t *)&candy_wrap_get_opcode(&self->opcode)[self->opcode.size - 1];
}
