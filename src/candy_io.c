/**
  * Copyright 2022-2023 ShunzDai
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
#include "src/candy_lib.h"
#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>

static int _vsnprint(candy_io_t *self, const char format[], va_list ap) {
  va_list head = ap;
  size_t len = vsnprintf(NULL, 0, format, ap) + 1;
  if (self->size < len) {
    free(self->buff);
    self->buff = calloc(len, sizeof(char));
  }
  return vsnprintf(self->buff, len, format, head);
}

int candy_io_try_catch(candy_io_t *self, candy_try_catch_cb_t cb, void *handle, void *ud) {
  if (setjmp(self->env))
    goto catch;
  cb(handle, ud);
  return 0;
  catch:
  printf("%s\n", self->buff);
  return -1;
}

void candy_io_throw(candy_io_t *self, const char format[], ...) {
  va_list ap;
  va_start(ap, format);
  _vsnprint(self, format, ap);
  va_end(ap);
  longjmp(self->env, 1);
}

void candy_io_expand(candy_io_t *self) {
  self->buff = (char *)expand(self->buff, sizeof(char), self->size, next_power2(self->size + 1), true);
  self->size = next_power2(self->size + 1);
}

int candy_io_init(candy_io_t *self) {
  self->buff = calloc(CANDY_DEFAULT_IO_SIZE, sizeof(char));
  self->size = CANDY_DEFAULT_IO_SIZE;
  return 0;
}

int candy_io_deinit(candy_io_t *self) {
  free(self->buff);
  self->buff = NULL;
  self->size = 0;
  return 0;
}
