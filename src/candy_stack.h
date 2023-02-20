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
#ifndef CANDY_SRC_STACK_H
#define CANDY_SRC_STACK_H
#ifdef __cplusplus
extern "C"{
#endif /* __cplusplus */

#include "src/candy_types.h"

typedef uint16_t candy_stack_size_t;

typedef struct candy_stack candy_stack_t;

candy_stack_t *candy_stack_create();
int candy_stack_delete(candy_stack_t **self);

candy_stack_size_t candy_stack_size(candy_stack_t *self);

candy_wrap_t *candy_stack_view(candy_stack_t *self, candy_stack_size_t idx);
candy_wrap_t *candy_stack_base(candy_stack_t *self);
candy_wrap_t *candy_stack_top(candy_stack_t *self);

void candy_stack_push(candy_stack_t *self, candy_wrap_t *data);
candy_wrap_t *candy_stack_pop(candy_stack_t *self);

candy_wraps_t candy_stack_type(candy_stack_t *self);

void candy_stack_push_integer(candy_stack_t *self, const candy_integer_t val);
void candy_stack_push_float(candy_stack_t *self, const candy_float_t val);
void candy_stack_push_boolean(candy_stack_t *self, const candy_boolean_t val);
void candy_stack_push_string(candy_stack_t *self, const char val[], size_t size);

candy_integer_t candy_stack_pull_integer(candy_stack_t *self);
candy_float_t candy_stack_pull_float(candy_stack_t *self);
candy_boolean_t candy_stack_pull_boolean(candy_stack_t *self);
const char *candy_stack_pull_string(candy_stack_t *self, size_t *size);

#ifdef __cplusplus
}
#endif /* __cplusplus */
#endif /* CANDY_SRC_STACK_H */

