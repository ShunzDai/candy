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
#ifndef CANDY_SRC_BUFFER_H
#define CANDY_SRC_BUFFER_H
#ifdef __cplusplus
extern "C"{
#endif /* __cplusplus */

#include "src/candy_types.h"
#include <stdlib.h>
#include <string.h>

typedef struct candy_buffer {
  int size;
  char *data;
} candy_buffer_t;

static inline int candy_buffer_get_size(candy_buffer_t *self) {
  return self->size;
}

static inline char *candy_buffer_get_data(candy_buffer_t *self) {
  return self->data;
}

static inline void candy_buffer_expand(candy_buffer_t *self) {
  char *data = (char *)calloc(self->size + CANDY_ATOMIC_BUFFER_SIZE, sizeof(char));
  memcpy(data, self->data, self->size);
  free(self->data);
  self->size += CANDY_ATOMIC_BUFFER_SIZE;
  self->data = data;
}

static inline int candy_buffer_init(candy_buffer_t *self) {
  self->size = CANDY_ATOMIC_BUFFER_SIZE;
  self->data = (char *)calloc(CANDY_ATOMIC_BUFFER_SIZE, sizeof(char));
  return 0;
}

static inline int candy_buffer_deinit(candy_buffer_t *self) {
  self->size = 0;
  free(self->data);
  self->data = NULL;
  return 0;
}

#ifdef __cplusplus
}
#endif /* __cplusplus */
#endif /* CANDY_SRC_BUFFER_H */
