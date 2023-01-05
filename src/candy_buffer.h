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
#ifndef CANDY_SRC_BUFFER_H
#define CANDY_SRC_BUFFER_H
#ifdef __cplusplus
extern "C"{
#endif /* __cplusplus */

#include "src/candy_types.h"
#include <stdlib.h>
#include <string.h>

typedef struct candy_buffer candy_buffer_t;

int candy_buffer_get_size(candy_buffer_t *self);

void *candy_buffer_get_data(candy_buffer_t *self);

void candy_buffer_expand(candy_buffer_t *self, int atomic, int size);

int candy_try_catch(candy_buffer_t *self, void (*cb)(void *), void *ud);

void candy_throw(candy_buffer_t *self, const char format[], ...) CANDY_NORETURN;

candy_buffer_t *candy_buffer_create(int atomic, int size, bool use_jmp);

int candy_buffer_delete(candy_buffer_t **self);

#ifdef __cplusplus
}
#endif /* __cplusplus */
#endif /* CANDY_SRC_BUFFER_H */
