/**
  * Copyright 2022-2023 ShunzDai
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
#include "src/candy_table.h"
#include "src/candy_io.h"
#include "src/candy_wrap.h"
#include <stdlib.h>
#include <string.h>

static inline size_t _size(const candy_wrap_t *self) {
  return self->size;
}

static inline candy_pair_t *_head(const candy_wrap_t *self) {
  return (candy_pair_t *)candy_wrap_get_table(self);
}

static inline candy_pair_t *_tail(const candy_wrap_t *self) {
  return _head(self) + _size(self) - 1;
}

static inline bool _boundary_check(candy_wrap_t *self, candy_pair_t *pair) {
  return (size_t)(pair - _head(self)) <= (_size(self) - 1);
}

static inline int32_t _get_next(int32_t now) {
  return ((now <= 0) - now) << 1;
}

static inline size_t hash_string(const char str[], size_t size) {
  size_t hash = size;
  size_t step = (size >> 5) + 1;
  for (size_t i = size; i >= step; i -= step)
    hash = hash ^ ((hash << 5) + (hash >> 2) + (size_t)str[i - 1]);
  return hash;
}

static size_t hash(const candy_wrap_t *key) {
  switch (key->type) {
    case TYPE_string:
      return hash_string(candy_wrap_get_string(key), key->size);
    default:
      return 0;
  }
}

static inline candy_pair_t *main_position(candy_wrap_t *self, const candy_wrap_t *key) {
  return _head(self) + hash(key) % _size(self);
}

static bool equal(const candy_wrap_t *keyl, const candy_wrap_t *keyr) {
  if (keyl->mask != keyr->mask)
    return false;
  return memcmp(candy_wrap_get_data(keyl), candy_wrap_get_data(keyr), keyl->size * candy_wrap_sizeof(keyl)) == 0;
}

int candy_table_fprint(const candy_wrap_t *self, FILE *out) {
  fprintf(out, "033[1;35m>>> table %p head\033[0m\n", self);
  fprintf(out, "pos  key-type         key-val  val-type         val-val\n");
  for (candy_pair_t *pair = _head(self); pair <= _tail(self); ++pair) {
    fprintf(out, "%3ld", pair - _head(self));
    fprintf(out, "%10s", candy_wrap_typestr(&pair->key));
    candy_wrap_fprint(&pair->key, out, 16);
    fprintf(out, "%10s", candy_wrap_typestr(&pair->val));
    candy_wrap_fprint(&pair->val, out, 16);
    fprintf(out, "\n");
  }
  fprintf(out, "\033[1;35m<<< table %p tail\033[0m\n", self);
  return 0;
}

const candy_wrap_t *candy_table_get(candy_wrap_t *self, const candy_wrap_t *key) {
  candy_pair_t *pair = main_position(self, key);
  for (int32_t next = 0; _boundary_check(self, pair + next); next += _get_next(next)) {
    if (equal(&(pair + next)->key, key))
      return &(pair + next)->val;
    else if ((pair + next)->key.type == TYPE_null)
      break;
  }
  return &null;
}

int candy_table_set(candy_wrap_t *self, const candy_wrap_t *key, const candy_wrap_t *val) {
  candy_pair_t *pair = main_position(self, key);
  for (int32_t next = 0; _boundary_check(self, pair + next); next += _get_next(next)) {
    switch ((pair + next)->key.type) {
      case TYPE_null:
        (pair + next)->key = *key;
        (pair + next)->val = *val;
        return 0;
      default:
        if (equal(&(pair + next)->key, key)) {
          (pair + next)->val = *val;
          return 0;
        }
        break;
    }
  }
  return -1;
}
