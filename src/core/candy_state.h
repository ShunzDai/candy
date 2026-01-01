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
#ifndef CANDY_CORE_STATE_H
#define CANDY_CORE_STATE_H
#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#include "core/candy_priv.h"

candy_state_t *candy_state_create(candy_handler_t handler, candy_allocator_t alloc, void *arg);

candy_state_t *candy_state_create_coroutine(candy_state_t *self);

candy_err_t candy_state_handler(candy_state_t *self, candy_gc_t *gc, candy_events_t evt, void *arg);

candy_err_t candy_state_close(candy_state_t *self);

candy_err_t candy_state_dostream(candy_state_t *self, candy_reader_t reader, void *arg);

candy_err_t candy_state_call(candy_state_t *self, int narg, int nres);

candy_err_t candy_state_setglobal(candy_state_t *self, const char name[], candy_cfunc_t entry);

candy_err_t candy_state_push_integer(candy_state_t *self, candy_integer_t val);

candy_err_t candy_state_push_float(candy_state_t *self, candy_float_t val);

candy_err_t candy_state_push_object(candy_state_t *self, const candy_object_t *val);

candy_err_t candy_state_push_cfunc(candy_state_t *self, candy_cfunc_t val);

candy_integer_t candy_state_to_integer(candy_state_t *self, int idx);

candy_float_t candy_state_to_float(candy_state_t *self, int idx);

candy_object_t *candy_state_to_object(candy_state_t *self, int idx);

candy_cfunc_t candy_state_to_cfunc(candy_state_t *self, int idx);

bool candy_state_is_primary(candy_state_t *self);

#ifdef __cplusplus
}
#endif /* __cplusplus */
#endif /* CANDY_CORE_STATE_H */
