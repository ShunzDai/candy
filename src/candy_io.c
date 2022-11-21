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
  if (self->r + CANDY_IO_LOOKAHEAD_SIZE == self->size) {
    char ahead[CANDY_IO_LOOKAHEAD_SIZE];
    memcpy(ahead, self->buffer + self->r, CANDY_IO_LOOKAHEAD_SIZE);
    self->reader(self->buffer + self->w + CANDY_IO_LOOKAHEAD_SIZE, self->size - self->w - CANDY_IO_LOOKAHEAD_SIZE, self->ud);
    memcpy(self->buffer + self->w, ahead, CANDY_IO_LOOKAHEAD_SIZE);
    self->r = self->w;
  }
  return self->buffer[self->r++];
}

void candy_io_write(candy_io_t *self, char ch) {
  self->buffer[self->w++] = ch;
  if (self->size - self->w < self->size / 4) {
    int size = self->size * 2;
    char *buffer = (char *)calloc(size, sizeof(char));
    memcpy(buffer, self->buffer, self->size);
    self->reader(buffer + self->size, self->size, self->ud);
    free(self->buffer);
    self->buffer = buffer;
    self->size = size;
  }
}

int candy_io_init(candy_io_t *self, candy_reader_t reader, void *ud) {
  memset(self, 0, sizeof(struct candy_io));
  self->reader = reader;
  self->ud = ud;
  self->buffer = (char *)calloc(CANDY_IO_DEFAULT_BUFFER_SIZE, sizeof(char));
  self->size = CANDY_IO_DEFAULT_BUFFER_SIZE;
  self->reader(self->buffer, self->size, self->ud);
  return 0;
}

int candy_io_deinit(candy_io_t *self) {
  free(self->buffer);
  self->buffer = NULL;
  return 0;
}
