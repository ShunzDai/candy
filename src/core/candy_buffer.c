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
#include "core/candy_buffer.h"
#include <string.h>

static const char TAG[] = "buffer";

static void *_wptr(candy_buffer_t *self) {
  return candy_vector_data(&self->vec) + self->w;
}

static const void *_rptr(candy_buffer_t *self) {
  return candy_vector_data(&self->vec) + self->r;
}

static int _fill(candy_buffer_t *self, candy_memory_t *mem, candy_excep_t *ctx, size_t ahead) {
  size_t sz = candy_vector_size(&self->vec);
  size_t cap = candy_vector_capacity(&self->vec);
  /* if the look-ahead step is smaller than the total length will be returned directly */
  if (self->r + ahead < sz)
    return 0;
  /* calculate the filling position of the read-only buffer */
  size_t offset = self->w + ahead;
  candy_logv(TAG, "w %zu r %zu sz %zu cap %zu offset %zu", self->w, self->r, sz, cap, offset);
  /** if the number of bytes that can be filled is less than
      @ref CANDY_CONFIG_BUFFER_EXPAND_SIZE bytes, the buffer will be enlarged */
  if (cap <= offset) {
    candy_vector_reserve(&self->vec, mem, ctx, cap + CANDY_CONFIG_BUFFER_EXPAND_SIZE, sizeof(char));
    candy_logw(TAG, "expanded from %zu to %zu", cap, candy_vector_capacity(&self->vec));
  }
  /* otherwise buffer will be filled directly */
  else {
    /* move the unread bytes to the head of the read-only buffer */
    memmove(_wptr(self), _rptr(self), ahead);
    candy_logd(TAG, "move %zu bytes from %zu to %zu", ahead, self->r, self->w);
    self->r = self->w;
  }
  /* fill buffer */
  int res = self->reader(candy_vector_data(&self->vec) + offset, candy_vector_capacity(&self->vec) - offset, self->arg);
  candy_logd(TAG, "fill %d bytes at %zu", res, offset);
  if (res > 0)
    candy_vector_resize(&self->vec, mem, ctx, offset + res, sizeof(char));
  return res;
}

candy_err_t candy_buffer_init(candy_buffer_t *self, candy_reader_t reader, void *arg) {
  candy_vector_init(&self->vec);
  self->w = 0;
  self->r = self->w;
  self->reader = reader;
  self->arg = arg;
  return CANDY_OK;
}

candy_err_t candy_buffer_deinit(candy_buffer_t *self, candy_memory_t *mem) {
  candy_vector_deinit(&self->vec, mem, sizeof(char));
  return CANDY_OK;
}

candy_err_t candy_buffer_reset(candy_buffer_t *self) {
  self->w = 0;
  return CANDY_OK;
}

int candy_buffer_view(candy_buffer_t *self, candy_memory_t *mem, candy_excep_t *ctx, void *data, size_t ahead) {
  size_t size = ahead;
  int res = 0;
  while ((res = _fill(self, mem, ctx, size)) > 0);
  if (res < 0)
    return res;
  memcpy(data, _rptr(self) + size, sizeof(char));
  return size;
}

int candy_buffer_read(candy_buffer_t *self, candy_memory_t *mem, candy_excep_t *ctx, void *data, size_t size) {
  int res = 0;
  while ((res = _fill(self, mem, ctx, size)) > 0);
  if (res < 0)
    return res;
  if (data)
    memcpy(data, _rptr(self), size);
  self->r += size;
  return size;
}

int candy_buffer_write(candy_buffer_t *self, const void *data, size_t size) {
  if (self->r < self->w + size)
    return -1;
  memcpy(_wptr(self), data, size);
  self->w += size;
  return size;
}

const void *candy_buffer_head(candy_buffer_t *self) {
  return candy_vector_data(&self->vec);
}

size_t candy_buffer_size(candy_buffer_t *self) {
  return self->w;
}
