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
#ifndef CANDY_SRC_OBJECT_H
#define CANDY_SRC_OBJECT_H
#ifdef __cplusplus
extern "C"{
#endif /* __cplusplus */

#include "src/candy_types.h"

void candy_object_print(candy_object_t *self);
int candy_object_add_object(candy_object_t *self, const candy_object_t *obj);
int candy_object_add_wrap(candy_object_t *self, const char name[], candy_wrap_t *wrap);
int candy_object_add_integer(candy_object_t *self, const char name[], const candy_integer_t val[], size_t size);
int candy_object_add_float(candy_object_t *self, const char name[], const candy_float_t val[], size_t size);
int candy_object_add_boolean(candy_object_t *self, const char name[], const candy_boolean_t val[], size_t size);
int candy_object_add_string(candy_object_t *self, const char name[], const char val[], size_t size);
int candy_object_add_cfunc(candy_object_t *self, const char name[], candy_cfunc_t func);

candy_object_t *candy_object_create(const char name[]);
int candy_object_delete(candy_object_t **self);

#ifdef __cplusplus
}
#endif /* __cplusplus */
#endif /* CANDY_SRC_OBJECT_H */
