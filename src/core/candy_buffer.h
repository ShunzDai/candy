/**
  * Copyright 2022-2026 ShunzDai
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

candy_err_t candy_buffer_init(candy_buffer_t *self, candy_reader_t reader, void *arg);

candy_err_t candy_buffer_deinit(candy_buffer_t *self, candy_memory_t *mem);

candy_err_t candy_buffer_reset(candy_buffer_t *self);

int candy_buffer_view(candy_buffer_t *self, candy_memory_t *mem, candy_excep_t *ctx, void *data, size_t ahead);

int candy_buffer_read(candy_buffer_t *self, candy_memory_t *mem, candy_excep_t *ctx, void *data, size_t size);

int candy_buffer_write(candy_buffer_t *self, const void *data, size_t size);

const void *candy_buffer_head(candy_buffer_t *self);

size_t candy_buffer_size(candy_buffer_t *self);

/* The unread span is valid until the next operation that refills the buffer. */
static inline size_t candy_buffer_span(const candy_buffer_t *self, const char **data) {
  size_t size = candy_vector_size(&self->vec);
  if (self->r >= size)
    return 0;
  *data = (const char *)candy_vector_data(&self->vec) + self->r;
  return size - self->r;
}

/* Consume and retain the first size bytes of the current unread span. */
static inline void candy_buffer_collect(candy_buffer_t *self, size_t size) {
  if (self->w != self->r) {
    char *data = (char *)candy_vector_data(&self->vec);
    memmove(data + self->w, data + self->r, size);
  }
  self->r += size;
  self->w += size;
}

/* The lexer touches one byte at a time, and almost always from bytes that are
 * already buffered: these let it copy/advance inline and only fall back to the
 * refilling path when the read cursor reaches the end of the buffered window. */
static inline bool candy_buffer_peek(candy_buffer_t *self, size_t ahead, char *ch) {
  if (self->r + ahead >= candy_vector_size(&self->vec))
    return false;
  *ch = ((const char *)candy_vector_data(&self->vec))[self->r + ahead];
  return true;
}

static inline bool candy_buffer_take(candy_buffer_t *self, size_t size, char *data) {
  if (self->r + size >= candy_vector_size(&self->vec))
    return false;
  if (data)
    memcpy(data, (const char *)candy_vector_data(&self->vec) + self->r, size);
  self->r += size;
  return true;
}

#ifdef __cplusplus
}
#endif /* __cplusplus */
#endif /* CANDY_CORE_BUFFER_H */
