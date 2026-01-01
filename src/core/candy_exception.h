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
#ifndef CANDY_CORE_EXCEPTION_H
#define CANDY_CORE_EXCEPTION_H
#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#include "core/candy_priv.h"

typedef void (*candy_excep_cb_t)(void *arg);

struct candy_excep {
  candy_excep_t *prev;
};

candy_err_t candy_excep_init(candy_excep_t *self);
candy_err_t candy_excep_deinit(candy_excep_t *self);

candy_err_t candy_excep_try(candy_excep_t *self, candy_excep_cb_t cb, void *arg, candy_object_t **err);
void candy_excep_throw(candy_excep_t *self, candy_err_t code, candy_object_t *err) CANDY_NORETURN;
size_t candy_excep_depth(const candy_excep_t *self);

#ifdef __cplusplus
}
#endif /* __cplusplus */
#endif /* CANDY_CORE_EXCEPTION_H */
