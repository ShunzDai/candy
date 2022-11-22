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
#include "src/candy_io.h"
#include <stdlib.h>
#include <string.h>
#include <assert.h>

char candy_io_view(candy_io_t *self, int idx) {
  assert(idx < CANDY_IO_LOOKAHEAD_SIZE);
  return self->buffer[self->r + idx];
}

char candy_io_read(candy_io_t *self) {
  /* only @ref CANDY_IO_LOOKAHEAD_SIZE bytes left to read */
  if (self->r + CANDY_IO_LOOKAHEAD_SIZE == self->size) {
    /* calculates the start position of the read buffer */
    int offset = self->w + CANDY_IO_LOOKAHEAD_SIZE + CANDY_IO_EXTRA_SIZE;
    /* if the number of bytes that can be filled is less than
       @ref CANDY_IO_ATOMIC_BUFFER_SIZE bytes, the buffer will be enlarged */
    if (self->size - offset < CANDY_IO_ATOMIC_BUFFER_SIZE) {
      char *buffer = (char *)calloc(self->size + CANDY_IO_ATOMIC_BUFFER_SIZE, sizeof(char));
      memcpy(buffer, self->buffer, self->size);
      free(self->buffer);
      self->reader(buffer + self->size, CANDY_IO_ATOMIC_BUFFER_SIZE, self->ud);
      self->buffer = buffer;
      self->size += CANDY_IO_ATOMIC_BUFFER_SIZE;
    }
    /* otherwise buffer will be filled directly */
    else {
      /* temporarily stores @ref CANDY_IO_LOOKAHEAD_SIZE bytes that have not been read */
      char ahead[CANDY_IO_LOOKAHEAD_SIZE];
      memcpy(ahead, self->buffer + self->r, CANDY_IO_LOOKAHEAD_SIZE);
      /* fill buffer */
      self->reader(self->buffer + offset, self->size - offset, self->ud);
      /* restore the unread @ref CANDY_IO_LOOKAHEAD_SIZE bytes to the buffer */
      memcpy(self->buffer + self->w + CANDY_IO_EXTRA_SIZE, ahead, CANDY_IO_LOOKAHEAD_SIZE);
      self->r = self->w + CANDY_IO_EXTRA_SIZE;
    }
  }
  return self->buffer[self->r++];
}

void candy_io_write(candy_io_t *self, char ch) {
  assert(self->r - self->w >= CANDY_IO_EXTRA_SIZE);
  self->buffer[self->w++] = ch;
}

int candy_io_init(candy_io_t *self, candy_reader_t reader, void *ud) {
  memset(self, 0, sizeof(struct candy_io));
  self->reader = reader;
  self->ud = ud;
  self->buffer = (char *)calloc(CANDY_IO_ATOMIC_BUFFER_SIZE, sizeof(char));
  self->size = CANDY_IO_ATOMIC_BUFFER_SIZE;
  self->r = CANDY_IO_EXTRA_SIZE;
  self->w = 0;
  self->reader(self->buffer + CANDY_IO_EXTRA_SIZE, self->size - CANDY_IO_EXTRA_SIZE, self->ud);
  return 0;
}

int candy_io_deinit(candy_io_t *self) {
  free(self->buffer);
  self->buffer = NULL;
  return 0;
}
