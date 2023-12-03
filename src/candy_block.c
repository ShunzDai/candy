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
#include "src/candy_object.h"
#include "src/candy_gc.h"
#include <stdlib.h>

candy_block_t *candy_block_new(candy_gc_t *gc) {
  candy_block_t *self = (candy_block_t *)candy_gc_add_pool(gc, type, sizeof(struct candy_block));

  return self;
}

int candy_block_delete(candy_block_t **self) {
  candy_wrap_deinit(&(*self)->ins);
  candy_wrap_deinit(&(*self)->pool);
  free(*self);
  *self = NULL;
  return 0;
}

int candy_block_add_const(candy_block_t *self, const candy_wrap_t *wrap) {
  candy_wrap_append(&self->pool, wrap, 1);
  return candy_wrap_size(&self->pool) - 1;
}

int candy_block_add_instruc(candy_block_t *self, candy_instruc_t *ins) {
  candy_wrap_append(&self->ins, ins, 1);
  return candy_wrap_size(&self->ins) - 1;
}
