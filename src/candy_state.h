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
#ifndef CANDY_SRC_STATE_H
#define CANDY_SRC_STATE_H
#ifdef __cplusplus
extern "C"{
#endif /* __cplusplus */

#include "src/candy_types.h"

candy_state_t *candy_state_create(void *ud);
int candy_state_delete(candy_state_t **self);

/**
  * @brief  get user's data.
  * @param  sta   candy state handle.
  * @return user's data.
  */
void *candy_ud(candy_state_t *self);

/**
  * @brief  register c-function entry.
  * @param  sta   candy state handle.
  * @param  entry if it is null, the low layer will use candy_regist_t::cfunc as a callback,
  *               otherwise the low layer will use candy_regist_t::cfunc as an argument to
  *               entry, users can customize the call method in entry, such as casting
  *               candy_regist_t::cfunc to a lambda expression in cpp.
  * @return none.
  */
void candy_regist_entry(candy_state_t *self, candy_cfunc_t entry);

/**
  * @brief  register c-function into a object.
  * @param  sta   candy state handle.
  * @param  obj   name of the object. if it does not exist, create it; otherwise use it.
  * @param  list  the list of c-functions to be registered in object.
  * @param  size  size of the list.
  * @return none.
  */
void candy_regist_cfuncs(candy_state_t *self, const char obj[], candy_regist_t list[], unsigned size);

candy_callinfo_t *candy_callinfo(candy_state_t *self);

void candy_push_integer(candy_state_t *self, candy_integer_t val);
void candy_push_float(candy_state_t *self, candy_float_t val);
void candy_push_boolean(candy_state_t *self, candy_boolean_t val);
void candy_push_string(candy_state_t *self, char *val, int size);

candy_integer_t candy_pull_integer(candy_state_t *self);
candy_float_t candy_pull_float(candy_state_t *self);
candy_boolean_t candy_pull_boolean(candy_state_t *self);
char *candy_pull_string(candy_state_t *self, int *size);

#ifdef __cplusplus
}
#endif /* __cplusplus */
#endif /* CANDY_SRC_STATE_H */
