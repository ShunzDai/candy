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
#ifndef CANDY_CORE_BUFFER_H
#define CANDY_CORE_BUFFER_H
#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#include "core/candy_vector.h"
#include "core/candy_priv.h"

typedef struct candy_buffer candy_buffer_t;

struct candy_buffer {
  candy_vector_t vec;
  size_t w;
  size_t r;
  candy_reader_t reader;
  void *arg;
};

int candy_buffer_init(candy_buffer_t *self, candy_reader_t reader, void *arg);

int candy_buffer_deinit(candy_buffer_t *self, candy_memory_t *mem);

int candy_buffer_view(candy_buffer_t *self, candy_memory_t *mem, candy_exce_t *ctx, void *data, size_t cell, size_t ahead);

int candy_buffer_read(candy_buffer_t *self, candy_memory_t *mem, candy_exce_t *ctx, void *data, size_t size);

int candy_buffer_write(candy_buffer_t *self, const void *data, size_t size);

const void *candy_buffer_head(candy_buffer_t *self);

size_t candy_buffer_size(candy_buffer_t *self);

void candy_buffer_reset(candy_buffer_t *self);

#ifdef __cplusplus
}
#endif /* __cplusplus */
#endif /* CANDY_CORE_BUFFER_H */
