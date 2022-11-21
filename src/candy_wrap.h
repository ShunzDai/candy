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
#ifndef CANDY_SRC_WRAP_H
#define CANDY_SRC_WRAP_H
#ifdef __cplusplus
extern "C"{
#endif /* __cplusplus */

#include "src/candy_types.h"
#include <stdlib.h>
#include <string.h>

union candy_wrap {
  struct {
    uint32_t type : 4;
    uint32_t      : 28;
  };
  struct {
    uint32_t      : 4;
    uint32_t size : 28;
    union {
      candy_integer_t sval[sizeof(long) / sizeof(candy_integer_t)];
      candy_integer_t *lval;
    };
  } i;
  struct {
    uint32_t      : 4;
    uint32_t size : 28;
    union {
      candy_float_t sval[sizeof(long) / sizeof(candy_float_t)];
      candy_float_t *lval;
    };
  } f;
  struct {
    uint32_t      : 4;
    uint32_t size : 28;
    union {
      candy_boolean_t sval[sizeof(long) / sizeof(candy_boolean_t)];
      candy_boolean_t *lval;
    };
  } b;
  struct {
    uint32_t      : 4;
    uint32_t size : 28;
    union {
      char sval[sizeof(long) / sizeof(char)];
      char *lval;
    };
  } s;
};

static inline bool candy_wrap_check_type(candy_wrap_t *self, candy_wraps_t type) {
  return self->type == type;
}

static inline bool candy_wrap_check_linteger(candy_wrap_t *self) {
  return self->i.size > sizeof(self->i.sval);
}

static inline bool candy_wrap_check_lfloat(candy_wrap_t *self) {
  return self->f.size > sizeof(self->f.sval);
}

static inline bool candy_wrap_check_lboolean(candy_wrap_t *self) {
  return self->b.size > sizeof(self->b.sval);
}

static inline bool candy_wrap_check_lstring(candy_wrap_t *self) {
  return self->s.size > sizeof(self->s.sval) - 1;
}

static inline candy_integer_t *candy_wrap_get_integer(candy_wrap_t *self, int *size) {
  if (size)
    *size = self->i.size;
  return candy_wrap_check_linteger(self) ? self->i.lval : self->i.sval;
}

static inline candy_float_t *candy_wrap_get_float(candy_wrap_t *self, int *size) {
  if (size)
    *size = self->f.size;
  return candy_wrap_check_lfloat(self) ? self->f.lval : self->f.sval;
}

static inline candy_boolean_t *candy_wrap_get_boolean(candy_wrap_t *self, int *size) {
  if (size)
    *size = self->b.size;
  return candy_wrap_check_lboolean(self) ? self->b.lval : self->b.sval;
}

static inline char *candy_wrap_get_string(candy_wrap_t *self, int *size) {
  if (size)
    *size = self->s.size;
  return candy_wrap_check_lstring(self) ? self->s.lval : self->s.sval;
}

static inline void candy_wrap_init_integer(candy_wrap_t *self, candy_integer_t *val, int size) {
  self->type = CANDY_INTEGER;
  self->i.size = size;
  if (candy_wrap_check_linteger(self)) {
    self->i.lval = (candy_integer_t *)calloc(size, sizeof(candy_integer_t));
    memcpy(self->i.lval, val, size * sizeof(candy_integer_t));
  }
  else {
    memcpy(self->i.sval, val, size * sizeof(candy_integer_t));
  }
}

static inline void candy_wrap_init_float(candy_wrap_t *self, candy_float_t *val, int size) {
  self->type = CANDY_FLOAT;
  self->f.size = size;
  if (candy_wrap_check_lfloat(self)) {
    self->f.lval = (candy_float_t *)calloc(size, sizeof(candy_float_t));
    memcpy(self->f.lval, val, size * sizeof(candy_float_t));
  }
  else {
    memcpy(self->f.sval, val, size * sizeof(candy_float_t));
  }
}

static inline void candy_wrap_init_boolean(candy_wrap_t *self, candy_boolean_t *val, int size) {
  self->type = CANDY_BOOLEAN;
  self->b.size = size;
  if (candy_wrap_check_lboolean(self)) {
    self->b.lval = (candy_boolean_t *)calloc(size, sizeof(candy_boolean_t));
    memcpy(self->b.lval, val, size * sizeof(candy_boolean_t));
  }
  else {
    memcpy(self->b.sval, val, size * sizeof(candy_boolean_t));
  }
}

static inline void candy_wrap_init_string(candy_wrap_t *self, char *val, int size) {
  self->type = CANDY_STRING;
  self->s.size = size;
  if (candy_wrap_check_lstring(self)) {
    self->s.lval = (char *)calloc(size + 1, sizeof(char));
    memcpy(self->s.lval, val, size * sizeof(char));
    self->s.lval[size] = 0;
  }
  else {
    memcpy(self->s.sval, val, size * sizeof(char));
    self->s.sval[size] = 0;
  }
}

static inline int candy_wrap_deinit(candy_wrap_t *self) {
  switch (self->type) {
    case CANDY_NONE:
      break;
    case CANDY_INTEGER:
      if (candy_wrap_check_linteger(self))
        free(self->i.lval);
      break;
    case CANDY_FLOAT:
      if (candy_wrap_check_lfloat(self))
        free(self->f.lval);
      break;
    case CANDY_BOOLEAN:
      if (candy_wrap_check_lboolean(self))
        free(self->b.lval);
      break;
    case CANDY_STRING:
      if (candy_wrap_check_lstring(self))
        free(self->s.lval);
      break;
    default:
      break;
  }
  return 0;
}

#ifdef __cplusplus
}
#endif /* __cplusplus */
#endif /* CANDY_SRC_WRAP_H */
