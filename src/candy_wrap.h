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

#include "src/candy_types.h"
#include <assert.h>

typedef enum candy_wraps {
#define CANDY_TYPE_ENUM
#include "src/candy_type.list"
} candy_wraps_t;

typedef CANDY_MASK_TYPE candy_mask_t;

struct candy_wrap {
  union {
    candy_mask_t mask;
    struct {
      candy_mask_t type : CANDY_BASIC_TYPE_SIZE;
      candy_mask_t size : sizeof(candy_mask_t) * 8 - CANDY_BASIC_TYPE_SIZE;
    };
  };
  candy_mask_t data[8 / sizeof(candy_mask_t)];
};

extern const candy_wrap_t null;

void candy_wrap_init(candy_wrap_t *self);
int candy_wrap_deinit(candy_wrap_t *self);

int candy_wrap_fprint(const candy_wrap_t *self, FILE *out, int align, int (table_fprint)(const candy_wrap_t *, FILE *));

void *candy_wrap_get_data(const candy_wrap_t *self);
void candy_wrap_set_data(candy_wrap_t *self, candy_wraps_t type, const void *data, int size);
void candy_wrap_append(candy_wrap_t *self, const void *data, int size);

static inline candy_wraps_t candy_wrap_type(const candy_wrap_t *self) {
  return (candy_wraps_t)self->type;
}

static inline int candy_wrap_size(const candy_wrap_t *self) {
  return self->size;
}

static inline const char *candy_wrap_typestr(const candy_wrap_t *self) {
  static const char *list[] = {
  #define CANDY_TYPE_STR
  #include "src/candy_type.list"
  };
  return list[self->type];
}

static inline int candy_wrap_sizeof(const candy_wrap_t *self) {
  static const int list[] = {
  #define CANDY_TYPE_SIZE
  #include "src/candy_type.list"
  };
  return list[self->type];
}

static inline const uint32_t *candy_wrap_get_uint32(const candy_wrap_t *self) {
  assert(self->type == TYPE_UINT32);
  return (const uint32_t *)candy_wrap_get_data(self);
}

static inline const candy_integer_t *candy_wrap_get_integer(const candy_wrap_t *self) {
  assert(self->type == TYPE_INTEGER);
  return (const candy_integer_t *)candy_wrap_get_data(self);
}

static inline const candy_float_t *candy_wrap_get_float(const candy_wrap_t *self) {
  assert(self->type == TYPE_FLOAT);
  return (const candy_float_t *)candy_wrap_get_data(self);
}

static inline const candy_boolean_t *candy_wrap_get_boolean(const candy_wrap_t *self) {
  assert(self->type == TYPE_BOOLEAN);
  return (const candy_boolean_t *)candy_wrap_get_data(self);
}

static inline const char *candy_wrap_get_string(const candy_wrap_t *self) {
  assert(self->type == TYPE_STRING);
  return (const char *)candy_wrap_get_data(self);
}

static inline const void **candy_wrap_get_ud(const candy_wrap_t *self) {
  assert(self->type == TYPE_USERDEF);
  return (const void **)candy_wrap_get_data(self);
}

static inline const candy_cfunc_t *candy_wrap_get_cfunc(const candy_wrap_t *self) {
  assert(self->type == TYPE_CFUNC);
  return (const candy_cfunc_t *)candy_wrap_get_data(self);
}

static inline const candy_wrap_t *candy_wrap_get_wrap(const candy_wrap_t *self) {
  assert(self->type == TYPE_WRAP);
  return (const candy_wrap_t *)candy_wrap_get_data(self);
}

static inline const candy_pair_t *candy_wrap_get_pair(const candy_wrap_t *self) {
  assert(self->type == TYPE_PAIR);
  return (const candy_pair_t *)candy_wrap_get_data(self);
}

static inline void candy_wrap_copy(candy_wrap_t *self, candy_wrap_t *src) {
  candy_wrap_set_data(self, (candy_wraps_t)src->type, candy_wrap_get_data(src), src->size);
}

static inline void candy_wrap_set_uint32(candy_wrap_t *self, const uint32_t *val, int size) {
  candy_wrap_set_data(self, TYPE_UINT32, val, size);
}

static inline void candy_wrap_set_integer(candy_wrap_t *self, const candy_integer_t *val, int size) {
  candy_wrap_set_data(self, TYPE_INTEGER, val, size);
}

static inline void candy_wrap_set_float(candy_wrap_t *self, const candy_float_t *val, int size) {
  candy_wrap_set_data(self, TYPE_FLOAT, val, size);
}

static inline void candy_wrap_set_boolean(candy_wrap_t *self, const candy_boolean_t *val, int size) {
  candy_wrap_set_data(self, TYPE_BOOLEAN, val, size);
}

static inline void candy_wrap_set_string(candy_wrap_t *self, const char *val, int size) {
  candy_wrap_set_data(self, TYPE_STRING, val, size);
}

static inline void candy_wrap_set_ud(candy_wrap_t *self, const void **val, int size) {
  candy_wrap_set_data(self, TYPE_USERDEF, val, size);
}

static inline void candy_wrap_set_cfunc(candy_wrap_t *self, const candy_cfunc_t *val, int size) {
  candy_wrap_set_data(self, TYPE_CFUNC, val, size);
}

static inline void candy_wrap_set_wrap(candy_wrap_t *self, const candy_wrap_t *val, int size) {
  candy_wrap_set_data(self, TYPE_WRAP, val, size);
}

static inline void candy_wrap_set_pair(candy_wrap_t *self, const candy_pair_t *val, int size) {
  candy_wrap_set_data(self, TYPE_PAIR, val, size);
}

#ifdef __cplusplus
}
#endif /* __cplusplus */
#endif /* CANDY_SRC_WRAP_H */
