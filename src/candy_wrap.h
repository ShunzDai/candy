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

typedef enum candy_wraps {
  CANDY_NONE,
  CANDY_INTEGER,
  CANDY_FLOAT,
  CANDY_BOOLEAN,
  CANDY_STRING,
  CANDY_BUILTIN,
  CANDY_USERDEF,
  CANDY_MAX,
} candy_wraps_t;

struct candy_wrap {
  uint32_t type :  4;
  uint32_t size : 28;
  union {
    uint64_t    : 64;
    void *data;
  };
};

typedef struct candy_wrap candy_wrap_t;

static inline bool candy_wrap_check_type(candy_wrap_t *self, candy_wraps_t type) {
  return self->type == type;
}

static inline bool candy_wrap_check_ldata(candy_wrap_t *self, size_t n) {
  return self->size > (sizeof(self->data) / n);
}

static inline void *candy_wrap_get_data(candy_wrap_t *self, int *size, size_t n) {
  if (size)
    *size = self->size;
  return candy_wrap_check_ldata(self, n) ? self->data : &self->data;
}

static inline void candy_wrap_set_data(candy_wrap_t *self, candy_wraps_t type, void *data, int size, size_t n) {
  self->type = type;
  self->size = size;
  memcpy(candy_wrap_check_ldata(self, n) ? (self->data = calloc(size, n)) : &self->data, data, size * n);
}

static inline candy_integer_t *candy_wrap_get_integer(candy_wrap_t *self, int *size) {
  return (candy_integer_t *)candy_wrap_get_data(self, size, sizeof(candy_integer_t));
}

static inline candy_float_t *candy_wrap_get_float(candy_wrap_t *self, int *size) {
  return (candy_float_t *)candy_wrap_get_data(self, size, sizeof(candy_float_t));
}

static inline candy_boolean_t *candy_wrap_get_boolean(candy_wrap_t *self, int *size) {
  return (candy_boolean_t *)candy_wrap_get_data(self, size, sizeof(candy_boolean_t));
}

static inline char *candy_wrap_get_string(candy_wrap_t *self, int *size) {
  return (char *)candy_wrap_get_data(self, size, sizeof(char));
}

static inline void **candy_wrap_get_ud(candy_wrap_t *self, int *size) {
  return (void **)candy_wrap_get_data(self, size, sizeof(void *));
}

static inline void candy_wrap_init_none(candy_wrap_t *self) {
  memset(self, 0, sizeof(struct candy_wrap));
}

static inline void candy_wrap_init_integer(candy_wrap_t *self, candy_integer_t *data, int size) {
  candy_wrap_set_data(self, CANDY_INTEGER, data, size, sizeof(candy_integer_t));
}

static inline void candy_wrap_init_float(candy_wrap_t *self, candy_float_t *data, int size) {
  candy_wrap_set_data(self, CANDY_FLOAT, data, size, sizeof(candy_float_t));
}

static inline void candy_wrap_init_boolean(candy_wrap_t *self, candy_boolean_t *data, int size) {
  candy_wrap_set_data(self, CANDY_BOOLEAN, data, size, sizeof(candy_boolean_t));
}

static inline void candy_wrap_init_string(candy_wrap_t *self, char *data, int size) {
  candy_wrap_set_data(self, CANDY_STRING, data, size, sizeof(char));
}

static inline void candy_wrap_init_ud(candy_wrap_t *self, void **data, int size) {
  candy_wrap_set_data(self, CANDY_STRING, data, size, sizeof(void *));
}

static inline int candy_wrap_deinit(candy_wrap_t *self) {
  switch (self->type) {
    case CANDY_NONE:
      break;
    case CANDY_INTEGER:
      if (candy_wrap_check_ldata(self, sizeof(candy_integer_t)))
        free(self->data);
      break;
    case CANDY_FLOAT:
      if (candy_wrap_check_ldata(self, sizeof(candy_float_t)))
        free(self->data);
      break;
    case CANDY_BOOLEAN:
      if (candy_wrap_check_ldata(self, sizeof(candy_boolean_t)))
        free(self->data);
      break;
    case CANDY_STRING:
      if (candy_wrap_check_ldata(self, sizeof(char)))
        free(self->data);
      break;
    default:
      break;
  }
  self->type = CANDY_NONE;
  return 0;
}

#ifdef __cplusplus
}
#endif /* __cplusplus */
#endif /* CANDY_SRC_WRAP_H */
