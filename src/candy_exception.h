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
#ifndef CANDY_SRC_EXCEPTION_H
#define CANDY_SRC_EXCEPTION_H
#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#include "src/candy_priv.h"

#define candy_assert(_self, _gc, _condition, _type, _format, ...) ((_condition) ? ((void)0U) : candy_exce_throw((_self), (_gc), #_type " error: " _format, ##__VA_ARGS__))

typedef void (*candy_exce_cb_t)(void *arg);

enum candy_exce_code {
  EXCE_OK,
};

typedef struct candy_exce candy_exce_t;

struct candy_exce {
  candy_exce_t *prev;
};

int candy_exce_init(candy_exce_t *self);
int candy_exce_deinit(candy_exce_t *self);

candy_array_t *candy_exce_try(candy_exce_t *self, candy_exce_cb_t cb, void *arg);
void candy_exce_throw(candy_exce_t *self, candy_gc_t *gc, const char format[], ...) CANDY_NORETURN;
size_t candy_exce_depth(const candy_exce_t *self);

#ifdef __cplusplus
}
#endif /* __cplusplus */
#endif /* CANDY_SRC_EXCEPTION_H */
