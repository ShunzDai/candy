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
#ifndef CANDY_CORE_WRAP_H
#define CANDY_CORE_WRAP_H
#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#include "core/candy_object.h"
#include "core/candy_priv.h"
#include <assert.h>

union candy_udata {
  candy_integer_t i;
  candy_float_t f;
  candy_boolean_t b;
  void *p;
};

struct candy_wrap {
  #if CANDY_MEMORY_ALIGNMENT
  union candy_udata data;
  #else /* CANDY_MEMORY_ALIGNMENT */
  uint8_t data[sizeof(union candy_udata)];
  #endif /* CANDY_MEMORY_ALIGNMENT */
  uint8_t type : 8;
  uint8_t mask : 4;
};

extern const candy_wrap_t CANDY_WRAP_NULL;

static inline const char *candy_type_str(candy_types_t type) {
  return (const char *[]) {
    #define CANDY_TYPE_STR
    #include "core/candy_type.list"
  }[type];
}

int candy_wrap_fprint(const candy_wrap_t *self, FILE *out, int align);

static inline void *candy_wrap_data(const candy_wrap_t *self) {
  return (void *)&self->data;
}

static inline candy_types_t candy_wrap_get_type(const candy_wrap_t *self) {
  return (candy_types_t)((self->type >> 0) & 0xFFU);
}

static inline candy_types_t candy_wrap_get_base(const candy_wrap_t *self) {
  return (candy_types_t)((self->type >> 0) & 0x0FU);
}

static inline candy_types_t candy_wrap_get_extd(const candy_wrap_t *self) {
  return (candy_types_t)((self->type >> 4) & 0x0FU);
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
  assert(candy_wrap_get_type(self) == CANDY_TYPE_INTEGER);
  assert(self->mask == MASK_NONE);
  return *(candy_integer_t *)candy_wrap_data(self);
}

static inline void candy_wrap_set_integer(candy_wrap_t *self, const candy_integer_t val) {
  candy_wrap_set_type(self, CANDY_TYPE_INTEGER);
  candy_wrap_set_mask(self, MASK_NONE);
  *(candy_integer_t *)candy_wrap_data(self) = val;
}

static inline candy_float_t candy_wrap_get_float(const candy_wrap_t *self) {
  assert(candy_wrap_get_type(self) == CANDY_TYPE_FLOAT);
  assert(self->mask == MASK_NONE);
  return *(candy_float_t *)candy_wrap_data(self);
}

static inline void candy_wrap_set_float(candy_wrap_t *self, const candy_float_t val) {
  candy_wrap_set_type(self, CANDY_TYPE_FLOAT);
  candy_wrap_set_mask(self, MASK_NONE);
  *(candy_float_t *)candy_wrap_data(self) = val;
}

static inline candy_boolean_t candy_wrap_get_boolean(const candy_wrap_t *self) {
  assert(candy_wrap_get_type(self) == CANDY_TYPE_BOOLEAN);
  assert(self->mask == MASK_NONE);
  return *(candy_boolean_t *)candy_wrap_data(self);
}

static inline void candy_wrap_set_boolean(candy_wrap_t *self, const candy_boolean_t val) {
  candy_wrap_set_type(self, CANDY_TYPE_BOOLEAN);
  candy_wrap_set_mask(self, MASK_NONE);
  *(candy_boolean_t *)candy_wrap_data(self) = val;
}

static inline candy_cfunc_t candy_wrap_get_cfunc(const candy_wrap_t *self) {
  assert(candy_wrap_get_type(self) == CANDY_TYPE_CFUNC);
  assert(self->mask == MASK_NONE);
  return *(candy_cfunc_t *)candy_wrap_data(self);
}

static inline void candy_wrap_set_cfunc(candy_wrap_t *self, const candy_cfunc_t val) {
  candy_wrap_set_type(self, CANDY_TYPE_CFUNC);
  candy_wrap_set_mask(self, MASK_NONE);
  *(candy_cfunc_t *)candy_wrap_data(self) = val;
}

static inline candy_object_t *candy_wrap_get_object(const candy_wrap_t *self) {
  assert(self->mask != MASK_NONE);
  return *(candy_object_t **)candy_wrap_data(self);
}

static inline void candy_wrap_set_object(candy_wrap_t *self, const candy_object_t *val) {
  candy_wrap_set_type(self, candy_object_get_type(val));
  candy_wrap_set_mask(self, candy_object_get_mask(val));
  *(const candy_object_t **)candy_wrap_data(self) = val;
}

#ifdef __cplusplus
}
#endif /* __cplusplus */
#endif /* CANDY_CORE_WRAP_H */
