/**
  * Copyright 2022-2026 ShunzDai
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
#ifndef CANDY_CORE_ARRAY_H
#define CANDY_CORE_ARRAY_H
#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#include "core/candy_priv.h"
#include <stdarg.h>

candy_array_t *candy_array_create(candy_gc_t *gc, candy_excep_t *ctx, candy_types_t type);

candy_array_t *candy_array_create_const(candy_gc_t *gc, candy_excep_t *ctx, candy_types_t type, const void *data, size_t size);

candy_array_t *candy_array_vprint(candy_gc_t *gc, candy_excep_t *ctx, const char format[], va_list args) CANDY_FORMAT(3, 0);

candy_array_t *candy_array_print(candy_gc_t *gc, candy_excep_t *ctx, const char format[], ...) CANDY_FORMAT(3, 4);

candy_err_t candy_array_handler(candy_array_t *self, candy_gc_t *gc, candy_events_t evt, void *arg);

size_t candy_array_capacity(const candy_array_t *self);

size_t candy_array_size(const candy_array_t *self);

void *candy_array_data(const candy_array_t *self);

void candy_array_reserve(candy_array_t *self, candy_gc_t *gc, candy_excep_t *ctx, size_t capacity);

void candy_array_resize(candy_array_t *self, candy_gc_t *gc, candy_excep_t *ctx, size_t size);

candy_err_t candy_array_append(candy_array_t *self, candy_gc_t *gc, candy_excep_t *ctx, const void *data, size_t size);

#ifdef __cplusplus
}
#endif /* __cplusplus */
#endif /* CANDY_CORE_ARRAY_H */
