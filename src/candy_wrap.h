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
#ifndef CANDY_SRC_WRAP_H
#define CANDY_SRC_WRAP_H
#ifdef __cplusplus
extern "C"{
#endif /* __cplusplus */

#include "src/candy_lib.h"
#include "src/candy_types.h"
#include <stdlib.h>
#include <string.h>
#include <assert.h>

struct candy_wrap {
  uint32_t data[2];
  uint32_t type :  4;
  uint32_t size : 28;
};

static inline size_t candy_wrap_sizeof(candy_wrap_t *self) {
  static const size_t list[] = {
#define CANDY_TYPE_SIZE
#include "candy_type.list"
  };
  assert(self->type < CANDY_MAX);
  return list[self->type];
}

static inline bool candy_wrap_check_ldata(candy_wrap_t *self, size_t n) {
  return self->size > ((sizeof(struct candy_wrap) - sizeof(uint32_t)) / n);
}

static inline void *candy_wrap_get_data(candy_wrap_t *self) {
  return candy_wrap_check_ldata(self, candy_wrap_sizeof(self)) ? *(void **)&self->data : (void *)&self->data;
}

static inline void candy_wrap_set_data(candy_wrap_t *self, candy_wraps_t type, const void *data, size_t size, size_t n) {
  self->type = type;
  self->size = size;
  memcpy(candy_wrap_check_ldata(self, n) ? (*(void **)&self->data = calloc(size, n)) : &self->data, data, size * n);
}

static inline candy_integer_t *candy_wrap_get_integer(candy_wrap_t *self) {
  assert(self->type == CANDY_INTEGER);
  return (candy_integer_t *)candy_wrap_get_data(self);
}

static inline candy_float_t *candy_wrap_get_float(candy_wrap_t *self) {
  assert(self->type == CANDY_FLOAT);
  return (candy_float_t *)candy_wrap_get_data(self);
}

static inline candy_boolean_t *candy_wrap_get_boolean(candy_wrap_t *self) {
  assert(self->type == CANDY_BOOLEAN);
  return (candy_boolean_t *)candy_wrap_get_data(self);
}

static inline char *candy_wrap_get_string(candy_wrap_t *self) {
  assert(self->type == CANDY_STRING);
  return (char *)candy_wrap_get_data(self);
}

static inline void **candy_wrap_get_ud(candy_wrap_t *self) {
  assert(self->type == CANDY_USERDEF);
  return (void **)candy_wrap_get_data(self);
}

static inline candy_object_t **candy_wrap_get_object(candy_wrap_t *self) {
  assert(self->type == CANDY_OBJECT);
  return (candy_object_t **)candy_wrap_get_data(self);
}

static inline candy_builtin_t *candy_wrap_get_builtin(candy_wrap_t *self) {
  assert(self->type == CANDY_BUILTIN);
  return (candy_builtin_t *)candy_wrap_get_data(self);
}

static inline void candy_wrap_init_none(candy_wrap_t *self) {
  memset(self, 0, sizeof(struct candy_wrap));
}

static inline void candy_wrap_init_integer(candy_wrap_t *self, const candy_integer_t val[], size_t size) {
  candy_wrap_set_data(self, CANDY_INTEGER, val, size, sizeof(candy_integer_t));
}

static inline void candy_wrap_init_float(candy_wrap_t *self, const candy_float_t val[], size_t size) {
  candy_wrap_set_data(self, CANDY_FLOAT, val, size, sizeof(candy_float_t));
}

static inline void candy_wrap_init_boolean(candy_wrap_t *self, const candy_boolean_t val[], size_t size) {
  candy_wrap_set_data(self, CANDY_BOOLEAN, val, size, sizeof(candy_boolean_t));
}

static inline void candy_wrap_init_string(candy_wrap_t *self, const char val[], size_t size) {
  candy_wrap_set_data(self, CANDY_STRING, val, size, sizeof(char));
}

static inline void candy_wrap_init_ud(candy_wrap_t *self, const void *val[], size_t size) {
  candy_wrap_set_data(self, CANDY_USERDEF, val, size, sizeof(void *));
}

static inline void candy_wrap_init_object(candy_wrap_t *self, const candy_object_t *val[], size_t size) {
  candy_wrap_set_data(self, CANDY_OBJECT, val, size, sizeof(candy_object_t *));
}

static inline void candy_wrap_init_builtin(candy_wrap_t *self, const candy_builtin_t val[], size_t size) {
  candy_wrap_set_data(self, CANDY_BUILTIN, val, size, sizeof(candy_builtin_t));
}

static inline int candy_wrap_deinit(candy_wrap_t *self) {
  if (candy_wrap_check_ldata(self, candy_wrap_sizeof(self)))
    free(*(void **)&self->data);
  memset(self, 0, sizeof(struct candy_wrap));
  return 0;
}

#ifdef __cplusplus
}
#endif /* __cplusplus */
#endif /* CANDY_SRC_WRAP_H */
