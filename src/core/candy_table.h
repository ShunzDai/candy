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
#ifndef CANDY_CORE_TABLE_H
#define CANDY_CORE_TABLE_H
#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#include "core/candy_priv.h"

#define CANDY_TABLE_CACHE_SIZE 8

typedef struct candy_table_slot {
  const candy_object_t *name;
  uint32_t index;
  uint32_t revision;
} candy_table_slot_t;

/* A VM-local cache for interned name keys, bound to one live table. */
typedef struct candy_table_cache {
  candy_table_t *table;
  candy_table_slot_t slots[CANDY_TABLE_CACHE_SIZE];
} candy_table_cache_t;

/* Compute a replacement value without mutating either table. */
typedef void (*candy_table_update_t)(candy_wrap_t *result, const candy_wrap_t *current, const candy_wrap_t *operand);

candy_table_t *candy_table_create(candy_gc_t *gc, candy_excep_t *ctx);

candy_table_t *candy_table_create_global(candy_gc_t *gc, candy_excep_t *ctx);

candy_err_t candy_table_handler(candy_table_t *self, candy_gc_t *gc, candy_events_t evt, void *arg);

candy_err_t candy_table_resize(candy_table_t *self, candy_gc_t *gc, candy_excep_t *ctx, size_t cap);

CANDY_LTO_INLINE_DECL const candy_wrap_t *candy_table_get(const candy_table_t *self, candy_gc_t *gc, const candy_wrap_t *key);

CANDY_LTO_INLINE_DECL candy_err_t candy_table_set(candy_table_t *self, candy_gc_t *gc, candy_excep_t *ctx, const candy_wrap_t *key, const candy_wrap_t *val);

void candy_table_cache_bind(candy_table_cache_t *cache, candy_table_t *table);

const candy_wrap_t *candy_table_get_cached(candy_table_cache_t *cache, candy_gc_t *gc, const candy_wrap_t *key);

/* A NULL update assigns the operand directly. Otherwise, read from fallback
 * only on a miss, and write the result to the bound table. */
candy_err_t candy_table_update_cached(candy_table_cache_t *cache, candy_gc_t *gc, candy_excep_t *ctx, const candy_wrap_t *key, const candy_table_t *fallback, candy_table_update_t update, const candy_wrap_t *operand);

candy_err_t candy_table_reset(candy_table_t *self, candy_gc_t *gc, const candy_wrap_t *key);

size_t candy_table_size(const candy_table_t *self, candy_gc_t *gc);

#ifdef __cplusplus
}
#endif /* __cplusplus */
#endif /* CANDY_CORE_TABLE_H */
