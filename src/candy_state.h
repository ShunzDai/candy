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
#ifndef CANDY_SRC_STATE_H
#define CANDY_SRC_STATE_H
#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#include "src/candy_types.h"

candy_state_t *candy_state_create(candy_allocator_t alloc, void *arg);
candy_state_t *candy_state_create_default(void);
candy_state_t *candy_state_create_coroutine(candy_state_t *self);
int candy_state_delete(candy_state_t *self);

int candy_state_dostream(candy_state_t *self, candy_reader_t reader, void *arg);
int candy_state_dostring(candy_state_t *self, const char exp[]);
int candy_state_dofile(candy_state_t *self, const char name[]);

bool candy_state_is_main(candy_state_t *self);

#ifdef __cplusplus
}
#endif /* __cplusplus */
#endif /* CANDY_SRC_STATE_H */
