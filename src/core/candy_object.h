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
#ifndef CANDY_CORE_OBJECT_H
#define CANDY_CORE_OBJECT_H
#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#include "core/candy_priv.h"

typedef enum candy_marks {
  MARK_WHITE,
  MARK_GRAY,
  MARK_DARK,
} candy_marks_t;

struct candy_object {
  #if CANDY_MEMORY_ALIGNMENT
  void *next;
  #else /* CANDY_MEMORY_ALIGNMENT */
  uint8_t next[sizeof(void *)];
  #endif /* CANDY_MEMORY_ALIGNMENT */
  uint8_t type : 8;
  uint8_t mask : 4;
  uint8_t mark : 4;
};

static inline candy_object_t **candy_object_get_next(candy_object_t *self) {
  return (candy_object_t **)&self->next;
}

static inline void candy_object_set_next(candy_object_t *self, candy_object_t *next) {
  *candy_object_get_next(self) = next;
}

static inline candy_types_t candy_object_get_type(const candy_object_t *self) {
  return (candy_types_t)((self->type >> 0) & 0xFFU);
}

static inline candy_types_t candy_object_get_base(const candy_object_t *self) {
  return (candy_types_t)((self->type >> 0) & 0x0FU);
}

static inline candy_types_t candy_object_get_extd(const candy_object_t *self) {
  return (candy_types_t)((self->type >> 4) & 0x0FU);
}

static inline void candy_object_set_type(candy_object_t *self, candy_types_t type) {
  self->type = type;
}

static inline uint8_t candy_object_get_mask(const candy_object_t *self) {
  return self->mask;
}

static inline void candy_object_set_mask(candy_object_t *self, uint8_t mask) {
  self->mask = mask;
}

static inline candy_marks_t candy_object_get_mark(const candy_object_t *self) {
  return (candy_marks_t)self->mark;
}

static inline void candy_object_set_mark(candy_object_t *self, candy_marks_t mark) {
  self->mark = mark;
}

#ifdef __cplusplus
}
#endif /* __cplusplus */
#endif /* CANDY_CORE_OBJECT_H */
