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
#include <assert.h>
#include <string.h>

/* Open addressing shared by the table and the GC string pool.
 * Callers supply slot classification, equality and hashing for their cell layout. */
#define CANDY_HASHMAP_LINEAR_MAX 8

typedef enum candy_hash_slot {
  CANDY_HASH_LIVE,
  CANDY_HASH_EMPTY,
  CANDY_HASH_TOMB,
} candy_hash_slot_t;

typedef struct candy_hash_ops {
  candy_hash_slot_t (*classify)(const void *cell);
  bool (*is_equal)(const void *cell, const void *key, candy_gc_t *gc);
  candy_hash_t (*hash)(const void *cell, candy_gc_t *gc);
} candy_hash_ops_t;

static CANDY_FORCE_INLINE bool candy_hashmap_grow(size_t used, size_t size) {
  /* Leave room for open-addressed probes without growing at half capacity. */
  return used + 1 > size - (size + 4) / 5;
}

/* Probe linearly for the first few slots (they share cache lines, which is what
 * most lookups need), then continue with a stride derived from the hash and kept
 * odd. The stride part is a bijection over a power-of-two table, so it still
 * covers every slot; it just leaves the long runs that small integers build
 * (they hash to themselves) in a step or two. */
static CANDY_FORCE_INLINE size_t candy_hashmap_step(candy_hash_t hash, size_t step, size_t mask) {
  uint32_t stride;
  uint64_t k;
  if (step <= CANDY_HASHMAP_LINEAR_MAX)
    return (size_t)(((uint64_t)(uint32_t)hash + step) & mask);
  stride = ((uint32_t)hash * 0x9E3779B1u) | 1u;
  k = step - CANDY_HASHMAP_LINEAR_MAX;
  return (size_t)(((uint64_t)(uint32_t)hash + CANDY_HASHMAP_LINEAR_MAX + k * stride) & mask);
}

/* Return a matching slot, or a vacant slot on a miss if insert is true.
 * A NULL key searches for a vacant slot without comparing keys. */
static CANDY_FORCE_INLINE void *candy_hashmap_find(void *data, size_t size, size_t cell, const candy_hash_ops_t *ops, const void *key, candy_hash_t hash, bool insert, candy_gc_t *gc) {
  if (data == NULL || size == 0)
    return NULL;
  size_t mask = size - 1;
  uint8_t *base = (uint8_t *)data;
  void *pos = base + cell * ((uint32_t)hash & mask);
  void *vacant = NULL;
  /* The linear prefix is followed by an odd-stride cycle. If the map has
   * no empty slots (e.g. after many deletes), allow the entire cycle. */
  for (size_t step = 1; ; ++step) {
    candy_hash_slot_t slot = ops->classify(pos);
    if (slot == CANDY_HASH_EMPTY)
      return insert ? (vacant ? vacant : pos) : NULL;
    if (slot == CANDY_HASH_TOMB) {
      if (key == NULL)
        return insert ? pos : NULL;
      if (insert && vacant == NULL)
        vacant = pos;
    }
    else if (key && ops->is_equal(pos, key, gc)) {
      return pos;
    }
    if (step == size + CANDY_HASHMAP_LINEAR_MAX)
      return vacant;
    pos = base + cell * candy_hashmap_step(hash, step, mask);
  }
}

static CANDY_FORCE_INLINE void *candy_hashmap_resize(void *data, size_t old_size, size_t new_size, size_t cell, const candy_hash_ops_t *ops, candy_memory_t *mem, candy_gc_t *gc, candy_excep_t *ctx) {
  void *next = candy_memory_alloc(mem, ctx, cell * new_size);
  memset(next, 0, cell * new_size);
  for (size_t index = 0; index < old_size; ++index) {
    const void *from = (const uint8_t *)data + cell * index;
    candy_hash_t hash;
    void *to;
    if (ops->classify(from) != CANDY_HASH_LIVE)
      continue;
    hash = ops->hash(from, gc);
    to = candy_hashmap_find(next, new_size, cell, ops, NULL, hash, true, gc);
    assert(to != NULL);
    memcpy(to, from, cell);
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
