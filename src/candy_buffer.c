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

#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>

static inline struct priv *_priv(candy_buffer_t *self) {
  return (struct priv *)self;
}

int candy_try_catch(candy_buffer_t *self, candy_try_catch_cb_t cb, void *handle, void *ud) {
  if (setjmp(self->env))
    goto catch;
  cb(handle, ud);
  return 0;
  catch:
  return -1;
}

void candy_throw(candy_buffer_t *self, const char format[], ...) {
  va_list ap;
  va_start(ap, format);
  size_t len = vsnprintf(NULL, 0, format, ap) + 1;
  va_end(ap);
  if (self->size < len) {
    free(self->data);
    self->data = calloc(len, sizeof(char));
  }
  va_start(ap, format);
  vsprintf(self->data, format, ap);
  va_end(ap);
  longjmp(self->env, 1);
}

void candy_buffer_expand(candy_buffer_t *self, size_t size, size_t n) {
  size_t old_size = self->size;
  void *old_data = self->data;
  self->size = self->size + size;
  self->data = calloc(self->size, n);
  memcpy(self->data, old_data, old_size * n);
  free(old_data);
  old_data = NULL;
}

candy_buffer_t *candy_buffer_create(size_t size, size_t n, bool use_jmp) {
  candy_buffer_t *self = (candy_buffer_t *)malloc(use_jmp ? sizeof(struct candy_buffer) : (sizeof(struct candy_buffer) - sizeof(jmp_buf)));
  self->size = size;
  self->data = calloc(self->size, n);
  return self;
}

int candy_buffer_delete(candy_buffer_t **self) {
  free((*self)->data);
  (*self)->data = NULL;
  free(_priv(*self));
  *self = NULL;
  return 0;
}
