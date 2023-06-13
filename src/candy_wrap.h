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

struct candy_pair {
  candy_wrap_t key;
  candy_wrap_t val;
};

extern const candy_wrap_t null;

int candy_wrap_fprint(const candy_wrap_t *self, FILE *out, int align);

void *candy_wrap_get_data(const candy_wrap_t *self);
void candy_wrap_set_data(candy_wrap_t *self, candy_wraps_t type, const void *data, size_t size);
void candy_wrap_append(candy_wrap_t *self, const void *data, size_t size);

void candy_wrap_init(candy_wrap_t *self);
int candy_wrap_deinit(candy_wrap_t *self);

static inline const char *candy_wrap_typestr(const candy_wrap_t *self) {
  static const char *list[] = {
  #define CANDY_TYPE_STR
  #include "src/candy_type.list"
  };
  return list[self->type];
}

static inline size_t candy_wrap_sizeof(const candy_wrap_t *self) {
  static const size_t list[] = {
  #define CANDY_TYPE_SIZE
  #include "src/candy_type.list"
  };
  return list[self->type];
}

static inline bool candy_wrap_check_ldata(const candy_wrap_t *self) {
  return self->size * candy_wrap_sizeof(self) > sizeof(self->data);
}

static inline void candy_wrap_copy(candy_wrap_t *self, candy_wrap_t *src) {
  candy_wrap_set_data(self, (candy_wraps_t)src->type, candy_wrap_get_data(src), src->size);
}

#define CANDY_TYPE_WRAP_FUNCS
#include "src/candy_type.list"

#ifdef __cplusplus
}
#endif /* __cplusplus */
#endif /* CANDY_SRC_WRAP_H */
