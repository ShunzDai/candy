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

struct candy_wrap {
  uint32_t data[2];
  union {
    uint32_t mask;
    struct {
      uint32_t type :  4;
      uint32_t size : 28;
    };
  };
};

extern const candy_wrap_t null;

void *candy_wrap_get_data(const candy_wrap_t *self);
void candy_wrap_set_data(candy_wrap_t *self, candy_wraps_t type, const void *data, size_t size);
void candy_wrap_append(candy_wrap_t *self, const void *data, size_t size);

void candy_wrap_init(candy_wrap_t *self);
int candy_wrap_deinit(candy_wrap_t *self);

static inline size_t candy_wrap_sizeof(const candy_wrap_t *self) {
  static const size_t list[] = {
#define CANDY_TYPE_SIZE
#include "src/candy_type.list"
  };
  return list[self->type];
}

static inline bool candy_wrap_check_ldata(const candy_wrap_t *self) {
  return self->size > (sizeof(self->data) / candy_wrap_sizeof(self));
}

static inline const candy_opcode_t *candy_wrap_get_opcode(const candy_wrap_t *self) {
  assert(self->type == CANDY_OPCODE);
  return (const candy_opcode_t *)candy_wrap_get_data(self);
}

static inline const candy_wrap_t *candy_wrap_get_wrap(const candy_wrap_t *self) {
  assert(self->type == CANDY_WRAP);
  return (const candy_wrap_t *)candy_wrap_get_data(self);
}

static inline const candy_integer_t *candy_wrap_get_integer(const candy_wrap_t *self) {
  assert(self->type == CANDY_INTEGER);
  return (const candy_integer_t *)candy_wrap_get_data(self);
}

static inline const candy_float_t *candy_wrap_get_float(const candy_wrap_t *self) {
  assert(self->type == CANDY_FLOAT);
  return (const candy_float_t *)candy_wrap_get_data(self);
}

static inline const candy_boolean_t *candy_wrap_get_boolean(const candy_wrap_t *self) {
  assert(self->type == CANDY_BOOLEAN);
  return (const candy_boolean_t *)candy_wrap_get_data(self);
}

static inline const char *candy_wrap_get_string(const candy_wrap_t *self) {
  assert(self->type == CANDY_STRING);
  return (const char *)candy_wrap_get_data(self);
}

static inline const void **candy_wrap_get_ud(const candy_wrap_t *self) {
  assert(self->type == CANDY_USERDEF);
  return (const void **)candy_wrap_get_data(self);
}

static inline const candy_builtin_t *candy_wrap_get_builtin(const candy_wrap_t *self) {
  assert(self->type == CANDY_BUILTIN);
  return (const candy_builtin_t *)candy_wrap_get_data(self);
}

static inline const candy_table_t **candy_wrap_get_table(const candy_wrap_t *self) {
  assert(self->type == CANDY_TABLE);
  return (const candy_table_t **)candy_wrap_get_data(self);
}

static inline void candy_wrap_set_opcode(candy_wrap_t *self, const candy_opcode_t val[], size_t size) {
  candy_wrap_set_data(self, CANDY_OPCODE, val, size);
}

static inline void candy_wrap_set_wrap(candy_wrap_t *self, const candy_wrap_t val[], size_t size) {
  candy_wrap_set_data(self, CANDY_WRAP, val, size);
}

static inline void candy_wrap_set_integer(candy_wrap_t *self, const candy_integer_t val[], size_t size) {
  candy_wrap_set_data(self, CANDY_INTEGER, val, size);
}

static inline void candy_wrap_set_float(candy_wrap_t *self, const candy_float_t val[], size_t size) {
  candy_wrap_set_data(self, CANDY_FLOAT, val, size);
}

static inline void candy_wrap_set_boolean(candy_wrap_t *self, const candy_boolean_t val[], size_t size) {
  candy_wrap_set_data(self, CANDY_BOOLEAN, val, size);
}

static inline void candy_wrap_set_string(candy_wrap_t *self, const char val[], size_t size) {
  candy_wrap_set_data(self, CANDY_STRING, val, size);
}

static inline void candy_wrap_set_ud(candy_wrap_t *self, const void *val[], size_t size) {
  candy_wrap_set_data(self, CANDY_USERDEF, val, size);
}

static inline void candy_wrap_set_builtin(candy_wrap_t *self, const candy_builtin_t val[], size_t size) {
  candy_wrap_set_data(self, CANDY_BUILTIN, val, size);
}

static inline void candy_wrap_set_table(candy_wrap_t *self, const candy_table_t *val[], size_t size) {
  candy_wrap_set_data(self, CANDY_TABLE, val, size);
}

static inline void candy_wrap_append_opcode(candy_wrap_t *self, const candy_opcode_t val[], size_t size) {
  candy_wrap_append(self, val, size);
}

static inline void candy_wrap_append_wrap(candy_wrap_t *self, const candy_wrap_t val[], size_t size) {
  candy_wrap_append(self, val, size);
}

static inline void candy_wrap_append_integer(candy_wrap_t *self, const candy_integer_t val[], size_t size) {
  candy_wrap_append(self, val, size);
}

static inline void candy_wrap_append_float(candy_wrap_t *self, const candy_float_t val[], size_t size) {
  candy_wrap_append(self, val, size);
}

static inline void candy_wrap_append_boolean(candy_wrap_t *self, const candy_boolean_t val[], size_t size) {
  candy_wrap_append(self, val, size);
}

static inline void candy_wrap_append_string(candy_wrap_t *self, const char val[], size_t size) {
  candy_wrap_append(self, val, size);
}

static inline void candy_wrap_append_ud(candy_wrap_t *self, const void *val[], size_t size) {
  candy_wrap_append(self, val, size);
}

static inline void candy_wrap_append_builtin(candy_wrap_t *self, const candy_builtin_t val[], size_t size) {
  candy_wrap_append(self, val, size);
}

static inline void candy_wrap_append_table(candy_wrap_t *self, const candy_table_t *val[], size_t size) {
  candy_wrap_append(self, val, size);
}

#ifdef __cplusplus
}
#endif /* __cplusplus */
#endif /* CANDY_SRC_WRAP_H */
