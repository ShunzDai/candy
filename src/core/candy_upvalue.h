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
#ifndef CANDY_CORE_UPVALUE_H
#define CANDY_CORE_UPVALUE_H
#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#include "core/candy_priv.h"

/**
  * @brief  create a closed upvalue holding a copy of `value`
  */
candy_upvalue_t *candy_upvalue_create_copy(candy_gc_t *gc, candy_excep_t *ctx, const candy_wrap_t *value);

candy_err_t candy_upvalue_handler(candy_upvalue_t *self, candy_gc_t *gc, candy_events_t evt, void *arg);

/**
  * @brief  the live value: the stack slot while open, the copy once closed
  */
candy_wrap_t *candy_upvalue_view(candy_upvalue_t *self);

/**
  * @brief  capture `slot` for the frame owning `list`, reusing an existing
  *         open upvalue when the slot is already captured
  */
candy_upvalue_t *candy_upvalue_open(candy_upvalue_t **list, candy_gc_t *gc, candy_excep_t *ctx, candy_vector_t *stack, ptrdiff_t slot);

/**
  * @brief  close every open upvalue whose slot lies at or above `from`
  */
void candy_upvalue_close_all(candy_upvalue_t **list, ptrdiff_t from);

#ifdef __cplusplus
}
#endif /* __cplusplus */
#endif /* CANDY_CORE_UPVALUE_H */
