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
#ifndef CANDY_SRC_STATE_H
#define CANDY_SRC_STATE_H
#ifdef __cplusplus
extern "C"{
#endif /* __cplusplus */

#include "src/candy_types.h"

candy_state_t *candy_state_create(void);
int candy_state_delete(candy_state_t **self);

int candy_dostring(candy_state_t *self, const char exp[]);

int candy_dofile(candy_state_t *self, const char name[]);

const char *candy_error(candy_state_t *self);

/**
  * @brief  register builtin-function into a object.
  * @param  self  candy state handle.
  * @param  name  name of the object. if it does not exist, create it; otherwise use it.
  * @param  list  the list of c-functions to be registered in object.
  * @param  size  size of the list.
  * @return none.
  */
int candy_regist(candy_state_t *self, const candy_regist_t list[], size_t size);

int candy_fprint(candy_state_t *self, FILE *out);
int candy_set_global(candy_state_t *self, const char name[]);
int candy_get_global(candy_state_t *self, const char name[]);
int candy_call(candy_state_t *self, int nargs, int nresults);

/**
  * @brief  get user's data.
  * @param  self  candy state handle.
  * @return user's data.
  */
void *candy_ud(candy_state_t *self);

void candy_push_integer(candy_state_t *self, const candy_integer_t val[], size_t size);
void candy_push_float(candy_state_t *self, const candy_float_t val[], size_t size);
void candy_push_boolean(candy_state_t *self, const candy_boolean_t val[], size_t size);
void candy_push_string(candy_state_t *self, const char val[], size_t size);
void candy_push_ud(candy_state_t *self, const void *val[], size_t size);
void candy_push_cfunc(candy_state_t *self, const candy_cfunc_t val[], size_t size);

const candy_integer_t *candy_pull_integer(candy_state_t *self, size_t *size);
const candy_float_t *candy_pull_float(candy_state_t *self, size_t *size);
const candy_boolean_t *candy_pull_boolean(candy_state_t *self, size_t *size);
const char *candy_pull_string(candy_state_t *self, size_t *size);
const void **candy_pull_ud(candy_state_t *self, size_t *size);
const candy_cfunc_t *candy_pull_cfunc(candy_state_t *self, size_t *size);

#ifdef __cplusplus
}
#endif /* __cplusplus */
#endif /* CANDY_SRC_STATE_H */
