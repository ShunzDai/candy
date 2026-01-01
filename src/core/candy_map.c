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
#include "core/candy_map.h"
#include "core/candy_memory.h"
#include "core/candy_lib.h"
#include <string.h>

static const char TAG[] = "map";

void *candy_map_find(const candy_map_t *self, candy_gc_t *gc, const void *key, candy_hash_t hash, bool expand) {
  const int32_t list[] = {3, -3, 5, -5, 7, -7, INT32_MAX};
  if (self->data == NULL) {
    return NULL;
  }
  for (size_t idx = 0; list[idx] != INT32_MAX; ++idx) {
    void *pos = self->data + self->cell * ((hash + list[idx]) & (capacity_to_size(self->cap) - 1));
    if (self->is_null(pos)) {
      /* if it is not expand, determine whether it is a tombstone */
      if (!expand) {
        /* if it is a tombstone, keep searching */
        if (self->is_tomb(pos))
          continue;
        /* otherwise, this key does not exist */
        break;
      }
      /* otherwise, it has been found */
    }
    else if (!self->comp(pos, key, gc)) {
      continue;
    }
    return pos;
  }
  return NULL;
}

candy_err_t candy_map_resize(candy_map_t *self, candy_gc_t *gc, candy_excep_t *ctx, uint8_t cap, candy_hash_t (*hash)(const void *key, candy_gc_t *gc)) {
  size_t psize = capacity_to_size(self->cap);
  size_t nsize = capacity_to_size(cap);
  candy_map_t m;
  m.is_null = self->is_null;
  m.is_tomb = self->is_tomb;
  m.comp = self->comp;
  m.cell = self->cell;
  m.cap = cap;
  m.data = candy_memory_alloc((candy_memory_t *)gc, ctx, self->cell * nsize);
  /** @attention can not longjmp begin */
  memset(m.data, 0, self->cell * nsize);
  for (size_t idx = 0; idx < psize; ++idx) {
    const void *from = self->data + self->cell * idx;
    if (self->is_null(from))
      continue;
    void *to = candy_map_find(&m, gc, from, hash(from, gc), true);
    memcpy(to, from, self->cell);
  }
  /** @attention can not longjmp end */
  candy_memory_free((candy_memory_t *)gc, self->data, self->cell * psize);
  *self = m;
  candy_logw(TAG, "resize %p, size from %zu to %zu", self, psize, nsize);
  return CANDY_OK;
}
