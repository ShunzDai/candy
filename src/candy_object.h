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
#ifndef CANDY_SRC_OBJECT_H
#define CANDY_SRC_OBJECT_H
#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#include "src/candy_types.h"

struct candy_object {
  uint8_t next[8];
  uint8_t type : 4;
  uint8_t mask : 4;
  uint8_t mark : 4;
};

static inline candy_types_t candy_object_get_type(const candy_object_t *self) {
  return (candy_types_t)self->type;
}

static inline void candy_object_set_type(candy_object_t *self, candy_types_t type) {
  self->type = type;
}

static inline candy_object_t *candy_object_get_next(candy_object_t *self) {
  return *(candy_object_t **)&self->next;
}

static inline void candy_object_set_next(candy_object_t *self, const candy_object_t *next) {
  *(const candy_object_t **)&self->next = next;
}

#ifdef __cplusplus
}
#endif /* __cplusplus */
#endif /* CANDY_SRC_OBJECT_H */