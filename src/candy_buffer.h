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
#ifndef CANDY_SRC_BUFFER_H
#define CANDY_SRC_BUFFER_H
#ifdef __cplusplus
extern "C"{
#endif /* __cplusplus */

#include "src/candy_types.h"
#include <setjmp.h>

#define candy_assert(_condition, _type, _format, ...) ((_condition) ? ((void)0U) : candy_buffer_throw(*(candy_buffer_t **)(self), #_type " error: " _format, ##__VA_ARGS__))

struct candy_buffer {
  char *buff;
  size_t size;
  jmp_buf env;
};

typedef void (*candy_try_catch_cb_t)(void *, void *);

int candy_buffer_try_catch(candy_buffer_t *self, candy_try_catch_cb_t cb, void *handle, void *ud);
void candy_buffer_throw(candy_buffer_t *self, const char format[], ...) CANDY_NORETURN;
void candy_buffer_expand(candy_buffer_t *self);

int candy_buffer_init(candy_buffer_t *self);
int candy_buffer_deinit(candy_buffer_t *self);

#ifdef __cplusplus
}
#endif /* __cplusplus */
#endif /* CANDY_SRC_BUFFER_H */
