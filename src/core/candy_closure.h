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
#ifndef CANDY_CORE_CLOSURE_H
#define CANDY_CORE_CLOSURE_H
#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#include "core/candy_priv.h"

/**
  * @brief  create a new c-closure
  */
candy_cclosure_t *candy_cclosure_create(candy_gc_t *gc, candy_exce_t *ctx, candy_cfunc_t cfunc);

int candy_cclosure_delete(candy_cclosure_t *self, candy_gc_t *gc);

/**
  * @brief  create a new script-closure
  */
candy_sclosure_t *candy_sclosure_create(candy_gc_t *gc, candy_exce_t *ctx, candy_proto_t *proto);

int candy_sclosure_delete(candy_sclosure_t *self, candy_gc_t *gc);

int candy_sclosure_colouring(candy_sclosure_t *self, candy_gc_t *gc);

int candy_sclosure_diffusion(candy_sclosure_t *self, candy_gc_t *gc);

const candy_proto_t *candy_closure_get_proto(candy_sclosure_t *self);

#ifdef __cplusplus
}
#endif /* __cplusplus */
#endif /* CANDY_CORE_CLOSURE_H */
