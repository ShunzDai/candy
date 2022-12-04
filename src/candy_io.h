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
#ifndef CANDY_SRC_IO_H
#define CANDY_SRC_IO_H
#ifdef __cplusplus
extern "C"{
#endif /* __cplusplus */

#include "src/candy_types.h"
#include <setjmp.h>

/** @ref doc/io_memory_model.drawio.png */
struct candy_io {
  jmp_buf rollback;
  candy_reader_t reader;
  void *ud;
  char *buffer;
  int size;
  int r;
  int w;
};

void candy_io_assert(candy_io_t *self, const char format[], ...) CANDY_NORETURN;

char candy_io_view(candy_io_t *self, int idx);
char candy_io_read(candy_io_t *self);
void candy_io_write(candy_io_t *self, char ch);

int candy_io_set_input(candy_io_t *self, candy_reader_t reader, void *ud);

int candy_io_init(candy_io_t *self);
int candy_io_deinit(candy_io_t *self);

#ifdef __cplusplus
}
#endif /* __cplusplus */
#endif /* CANDY_SRC_IO_H */
