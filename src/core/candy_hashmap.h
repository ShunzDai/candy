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
#ifndef CANDY_CORE_HASHMAP_H
#define CANDY_CORE_HASHMAP_H
#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#include "core/candy_priv.h"
#include "core/candy_memory.h"
#include <string.h>

/* Open addressing shared by the table and the GC string pool.
 * The cell layout stays typed; only empty/tomb/is_equal/hash are supplied by the
 * caller through a compile-time constant ops table. */
typedef struct candy_hash_ops {
  bool (*is_empty)(const void *cell);
  bool (*is_tomb)(const void *cell);
  bool (*is_equal)(const void *cell, const void *key, candy_gc_t *gc);
  candy_hash_t (*hash)(const void *cell, candy_gc_t *gc);
} candy_hash_ops_t;

static CANDY_FORCE_INLINE bool candy_hashmap_grow(size_t used, size_t size) {
  return size == 0 || (used + 1) * 2 > size;
}

static CANDY_FORCE_INLINE void *candy_hashmap_find(void *data, size_t size, size_t cell, const candy_hash_ops_t *ops, const void *key, candy_hash_t hash, bool expand, candy_gc_t *gc) {
  if (data == NULL || size == 0)
    return NULL;
  size_t mask = size - 1;
  uint8_t *base = (uint8_t *)data;
  void *vacant = NULL;
  for (size_t step = 0; step < size; ++step) {
    void *pos = base + cell * ((hash + step) & mask);
    if (ops->is_empty(pos)) {
      if (vacant == NULL)
        vacant = pos;
      break;
    }
    if (ops->is_tomb(pos)) {
      if (vacant == NULL)
        vacant = pos;
      continue;
    }
    if (ops->is_equal(pos, key, gc))
      return pos;
  }
  return expand ? vacant : NULL;
}

static CANDY_FORCE_INLINE void *candy_hashmap_resize(void *data, size_t old_size, size_t new_size, size_t cell, const candy_hash_ops_t *ops, candy_memory_t *mem, candy_gc_t *gc, candy_excep_t *ctx) {
  void *next = candy_memory_alloc(mem, ctx, cell * new_size);
  memset(next, 0, cell * new_size);
  for (size_t index = 0; index < old_size; ++index) {
    const void *from = (const uint8_t *)data + cell * index;
    if (ops->is_empty(from) || ops->is_tomb(from))
      continue;
    candy_hash_t hash = ops->hash(from, gc);
    for (size_t step = 0; ; ++step) {
      void *to = (uint8_t *)next + cell * ((hash + step) & (new_size - 1));
      if (ops->is_empty(to)) {
        memcpy(to, from, cell);
        break;
      }
    }
  }
  candy_memory_free(mem, data, cell * old_size);
  return next;
}

static CANDY_FORCE_INLINE void candy_hashmap_deinit(void *data, size_t size, size_t cell, candy_memory_t *mem) {
  candy_memory_free(mem, data, cell * size);
}

#ifdef __cplusplus
}
#endif /* __cplusplus */
#endif /* CANDY_CORE_HASHMAP_H */
