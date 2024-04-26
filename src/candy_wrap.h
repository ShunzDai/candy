/**
  * Copyright 2022-2024 ShunzDai
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
extern "C" {
#endif /* __cplusplus */

#include "src/candy_object.h"
#include "src/candy_priv.h"
#include <assert.h>

#define CANDY_WRAP_NULL (candy_wrap_t){0}

typedef enum candy_mask {
  MASK_NONE,
  MASK_OBJECT   = 1 << 0,
  MASK_CCLOSURE = 1 << 1,
  MASK_SCLOSURE = 1 << 2,
  MASK_CFUNCPTR = 1 << 3,
} candy_mask_t;

struct candy_wrap {
  uint8_t data[8];
  uint8_t type : 4;
  uint8_t mask : 4;
};

static inline const char *candy_type_str(candy_types_t type) {
  return (const char *[]) {
    #define CANDY_TYPE_STR
    #include "src/candy_type.list"
  }[type];
}

int candy_wrap_fprint(const candy_wrap_t *self, FILE *out, int align);

static inline void *candy_wrap_data(const candy_wrap_t *self) {
  return (void *)&self->data;
}

static inline candy_types_t candy_wrap_get_type(const candy_wrap_t *self) {
  return (candy_types_t)self->type;
}

static inline void candy_wrap_set_type(candy_wrap_t *self, candy_types_t type) {
  self->type = type;
}

static inline uint8_t candy_wrap_get_mask(const candy_wrap_t *self) {
  return self->mask;
}

static inline void candy_wrap_set_mask(candy_wrap_t *self, uint8_t mask) {
  self->mask = mask;
}

static inline candy_integer_t candy_wrap_get_integer(const candy_wrap_t *self) {
  assert(candy_wrap_get_type(self) == TYPE_INTEGER);
  return *(candy_integer_t *)candy_wrap_data(self);
}

static inline candy_float_t candy_wrap_get_float(const candy_wrap_t *self) {
  assert(candy_wrap_get_type(self) == TYPE_FLOAT);
  return *(candy_float_t *)candy_wrap_data(self);
}

static inline candy_boolean_t candy_wrap_get_boolean(const candy_wrap_t *self) {
  assert(candy_wrap_get_type(self) == TYPE_BOOLEAN);
  return *(candy_boolean_t *)candy_wrap_data(self);
}

static inline candy_object_t *candy_wrap_get_object(const candy_wrap_t *self) {
  assert(self->mask & MASK_OBJECT);
  return *(candy_object_t **)candy_wrap_data(self);
}

static inline void candy_wrap_set_integer(candy_wrap_t *self, const candy_integer_t val) {
  candy_wrap_set_type(self, TYPE_INTEGER);
  *(candy_integer_t *)candy_wrap_data(self) = val;
}

static inline void candy_wrap_set_float(candy_wrap_t *self, const candy_float_t val) {
  candy_wrap_set_type(self, TYPE_FLOAT);
  *(candy_float_t *)candy_wrap_data(self) = val;
}

static inline void candy_wrap_set_boolean(candy_wrap_t *self, const candy_boolean_t val) {
  candy_wrap_set_type(self, TYPE_BOOLEAN);
  *(candy_boolean_t *)candy_wrap_data(self) = val;
}

static inline void candy_wrap_set_object(candy_wrap_t *self, const candy_object_t *val) {
  candy_wrap_set_type(self, candy_object_get_type(val));
  candy_wrap_set_mask(self, MASK_OBJECT);
  *(const candy_object_t **)candy_wrap_data(self) = val;
}

#ifdef __cplusplus
}
#endif /* __cplusplus */
#endif /* CANDY_SRC_WRAP_H */
