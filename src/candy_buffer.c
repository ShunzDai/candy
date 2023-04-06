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
#include <stdlib.h>
#include <string.h>

struct priv {
  void *data;
  size_t size;
  jmp_buf env;
};

static inline struct priv *_priv(candy_buffer_t *self) {
  return (struct priv *)self;
}

int candy_try_catch(candy_buffer_t *self, candy_try_catch_cb_t cb, void *handle, void *ud) {
  if (setjmp(_priv(self)->env))
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
  if (_priv(self)->size < len) {
    free(_priv(self)->data);
    _priv(self)->data = calloc(len, sizeof(char));
  }
  va_start(ap, format);
  vsprintf(_priv(self)->data, format, ap);
  va_end(ap);
  longjmp(_priv(self)->env, 1);
}

void candy_buffer_expand(candy_buffer_t *self, int size, int n) {
  size_t old_size = _priv(self)->size;
  void *old_data = _priv(self)->data;
  _priv(self)->size = _priv(self)->size + size;
  _priv(self)->data = calloc(_priv(self)->size, n);
  memcpy(_priv(self)->data, old_data, old_size * n);
  free(old_data);
  old_data = NULL;
}

candy_buffer_t *candy_buffer_create(int size, int n, bool use_jmp) {
  candy_buffer_t *self = (candy_buffer_t *)malloc(use_jmp ? sizeof(struct priv) : (sizeof(struct priv) - sizeof(jmp_buf)));
  _priv(self)->size = size;
  _priv(self)->data = calloc(_priv(self)->size, n);
  return self;
}

int candy_buffer_delete(candy_buffer_t **self) {
  free(_priv(*self)->data);
  _priv(*self)->data = NULL;
  free(_priv(*self));
  *self = NULL;
  return 0;
}
