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
#include "core/candy_buffer.h"
#include <string.h>

static char *_data(candy_buffer_t *self) {
  return (char *)candy_vector_data(&self->vec);
}

static char *_wptr(candy_buffer_t *self) {
  return _data(self) + self->w;
}

static char *_rptr(candy_buffer_t *self) {
  return _data(self) + self->r;
}

static size_t _size(candy_buffer_t *self) {
  return candy_vector_size(&self->vec);
}

static int _fill(candy_buffer_t *self, candy_gc_t *gc, size_t ahead) {
  size_t size = _size(self);
  /* if the look-ahead step is smaller than the total length will be returned directly */
  if (self->r + ahead < size)
    return 0;
  /* calculate the filling position of the read-only buffer */
  size_t offset = self->w + ahead;
  /** if the number of bytes that can be filled is less than
      @ref CANDY_BUFFER_EXPAND_SIZE bytes, the buffer will be enlarged */
  if (size < CANDY_BUFFER_EXPAND_SIZE + offset) {
    candy_vector_append(&self->vec, gc, NULL, CANDY_BUFFER_EXPAND_SIZE);
    offset = size;
  }
  /* otherwise buffer will be filled directly */
  else {
    /* restore the unread remain bytes to the header of the read-only buffer */
    memmove(_wptr(self), _rptr(self), ahead);
    self->r = self->w;
  }
  /* fill buffer */
  int res = self->reader(_data(self) + offset, _size(self) - offset, self->arg);
  if (res > 0)
    candy_vector_resize(&self->vec, gc, offset + res);
  return res;
}

int candy_buffer_init(candy_buffer_t *self, candy_reader_t reader, void *arg) {
  candy_vector_init(&self->vec, sizeof(char));
  self->w = 0;
  self->r = self->w;
  self->reader = reader;
  self->arg = arg;
  return 0;
}

int candy_buffer_deinit(candy_buffer_t *self, candy_gc_t *gc) {
  candy_vector_deinit(&self->vec, gc);
  return 0;
}

int candy_buffer_view(candy_buffer_t *self, candy_gc_t *gc, void *data, size_t cell, size_t ahead) {
  size_t size = cell * ahead;
  int res = 0;
  while ((res = _fill(self, gc, cell * ahead)) > 0);
  if (res < 0)
    return res;
  memcpy(data, _rptr(self) + size, cell);
  return res;
}

int candy_buffer_read(candy_buffer_t *self, candy_gc_t *gc, void *data, size_t size) {
  int res = 0;
  while ((res = _fill(self, gc, size - 1)) > 0);
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
  return _data(self);
}

size_t candy_buffer_size(candy_buffer_t *self) {
  return self->w;
}

void candy_buffer_reset(candy_buffer_t *self) {
  self->w = 0;
}
