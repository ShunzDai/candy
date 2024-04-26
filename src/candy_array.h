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
#ifndef CANDY_SRC_ARRAY_H
#define CANDY_SRC_ARRAY_H
#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#include "src/candy_priv.h"

candy_array_t *candy_array_create(candy_gc_t *self, candy_types_t type, size_t cell);
int candy_array_delete(candy_array_t *self, candy_gc_t *gc);

size_t candy_array_capacity(const candy_array_t *self);
size_t candy_array_size(const candy_array_t *self);
void *candy_array_data(const candy_array_t *self);
candy_vector_t *candy_array_vector(candy_array_t *self);

void candy_array_resize(candy_array_t *self, candy_gc_t *gc, size_t size);

int candy_array_append(candy_array_t *self, candy_gc_t *gc, const void *data, size_t size);

#ifdef __cplusplus
}
#endif /* __cplusplus */
#endif /* CANDY_SRC_ARRAY_H */
