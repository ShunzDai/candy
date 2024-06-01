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
#include "core/candy_exception.h"
#include <setjmp.h>

struct context {
  jmp_buf jmp;
  struct context *prev;
  candy_object_t *err;
};

int candy_exce_init(candy_exce_t *self) {
  self->prev = NULL;
  return 0;
}

int candy_exce_deinit(candy_exce_t *self) {
  return 0;
}

candy_object_t *candy_exce_try(candy_exce_t *self, candy_exce_cb_t cb, void *arg) {
  struct context new = {
    .prev = (struct context *)self->prev,
  };
  self->prev = (candy_exce_t *)&new;
  if (setjmp(new.jmp))
    goto catch;
  cb(arg);
  catch:
  self->prev = (candy_exce_t *)new.prev;
  return new.err;
}

void candy_exce_throw(candy_exce_t *self, candy_object_t *err) {
  struct context *ctx = (struct context *)self->prev;
  ctx->err = err;
  longjmp(ctx->jmp, 1);
}

size_t candy_exce_depth(const candy_exce_t *self) {
  struct context *ctx = (struct context *)self->prev;
  size_t depth = 0;
  for (const struct context *it = ctx; it; ++depth, it = it->prev);
  return depth;
}
