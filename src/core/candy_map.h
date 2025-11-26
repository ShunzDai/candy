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
  void *data;
  uint8_t cap;
};

#define candy_map_find(_type) { \
  const int32_t list[] = {3, -3, 5, -5, 7, -7, INT32_MAX}; \
  for (size_t idx = 0; list[idx] != INT32_MAX; ++idx) { \
    _type *pos = (_type *)self->data + ((hash + list[idx]) & (capacity_to_size(self->cap) - 1)); \
    if (_is_none(pos)) { \
      /* if it is view mode, determine whether it is a tombstone */ \
      if (view) { \
        /* if it is a tombstone, keep searching */ \
        if (_is_tomb(pos)) \
          continue; \
        /* otherwise, this key does not exist */ \
        break; \
      } \
      /* otherwise, it has been found */ \
    } \
    else if (!_comp(pos, key, gc)) { \
      continue; \
    } \
    return pos; \
  } \
  return NULL; \
}

#define candy_map_resize(_type) { \
  size_t nsize = capacity_to_size(cap); \
  size_t psize = capacity_to_size(self->cap); \
  candy_map_t m; \
  m.cap = cap; \
  m.data = candy_memory_alloc(candy_gc_memory(gc), ctx, sizeof(_type) * nsize); \
  memset(m.data, 0, sizeof(_type) * nsize); \
  for (size_t idx = 0; idx < psize; ++idx) { \
    _type *from = (_type *)self->data + idx; \
    if (_is_none(from)) \
      continue; \
    _type *to = _find(&m, gc, _key(from, gc), false); \
    *to = *from; \
  } \
  candy_memory_free(candy_gc_memory(gc), self->data, sizeof(_type) * psize); \
  *self = m; \
  candy_logw(TAG, "resize %p, size from %zu to %zu", self, psize, nsize); \
  return CANDY_OK; \
}

#ifdef __cplusplus
}
#endif /* __cplusplus */
#endif /* CANDY_CORE_MAP_H */
