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
#include "src/candy_buffer.h"
#include <setjmp.h>
#include <stdio.h>
#include <stdarg.h>

struct candy_buffer {
  int size;
  void *data;
  jmp_buf env;
};

int candy_buffer_get_size(candy_buffer_t *self) {
  return self->size;
}

void *candy_buffer_get_data(candy_buffer_t *self) {
  return self->data;
}

int candy_try_catch(candy_buffer_t *self, void (*cb)(void *), void *ud) {
  if(setjmp(self->env))
    goto catch;
  cb(ud);
  return 0;
  catch:
  return -1;
}

void candy_throw(candy_buffer_t *self, const char format[], ...) {
  va_list ap;
  va_start(ap, format);
  vsnprintf(self->data, self->size, format, ap);
  va_end(ap);
  longjmp(self->env, 1);
}

void candy_buffer_expand(candy_buffer_t *self, int atomic, int size) {
  void *data = calloc(self->size + atomic, size);
  memcpy(data, self->data, self->size * size);
  free(self->data);
  self->size += atomic;
  self->data = data;
}

candy_buffer_t *candy_buffer_create(int atomic, int size, bool use_jmp) {
  candy_buffer_t *self = (candy_buffer_t *)malloc(use_jmp ? sizeof(struct candy_buffer) : (sizeof(struct candy_buffer) - sizeof(jmp_buf)));
  self->size = atomic;
  self->data = calloc(atomic, size);
  return self;
}

int candy_buffer_delete(candy_buffer_t **self) {
  free((*self)->data);
  (*self)->data = NULL;
  free(*self);
  *self = NULL;
  return 0;
}
