/**
  * Copyright 2022-2025 ShunzDai
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
#ifndef CANDY_CORE_MAP_H
#define CANDY_CORE_MAP_H
#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#include "core/candy_priv.h"

typedef struct candy_map candy_map_t;

struct candy_map {
  bool (*is_null)(const void *pos);
  bool (*is_tomb)(const void *pos);
  bool (*comp)(const void *pos, const void *key, candy_gc_t *gc);
  void *data;
  size_t cell;
  uint8_t cap;
};

void *candy_map_find(const candy_map_t *self, candy_gc_t *gc, const void *key, candy_hash_t hash, bool expand);

candy_err_t candy_map_resize(candy_map_t *self, candy_gc_t *gc, candy_excep_t *ctx, uint8_t cap, candy_hash_t (*hash)(const void *key, candy_gc_t *gc));

#ifdef __cplusplus
}
#endif /* __cplusplus */
#endif /* CANDY_CORE_MAP_H */
