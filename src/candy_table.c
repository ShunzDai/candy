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
#include "src/candy_wrap.h"
#include "src/candy_gc.h"
#include "src/candy_lib.h"
#include <string.h>

struct candy_pair {
  candy_wrap_t key;
  candy_wrap_t val;
};

struct candy_table {
  candy_wrap_t gcobj;
  candy_wrap_t pair;
  size_t size;
};

static inline int _size(const candy_table_t *self) {
  return self->size;
}

static inline candy_pair_t *_head(const candy_table_t *self) {
  return (candy_pair_t *)candy_wrap_get_ldata(&self->pair);
}

static inline candy_pair_t *_tail(const candy_table_t *self) {
  return _head(self) + _size(self) - 1;
}

static inline bool _boundary_check(const candy_table_t *self, candy_pair_t *pair) {
  return (int)(pair - _head(self)) <= (_size(self) - 1);
}

static inline int32_t _get_next(int32_t now) {
  return ((now <= 0) - now) << 1;
}

// static inline int hash_string(const char str[], int size) {
//   int hash = size;
//   int step = (size >> 5) + 1;
//   for (int i = size; i >= step; i -= step)
//     hash = hash ^ ((hash << 5) + (hash >> 2) + (int)str[i - 1]);
//   return hash;
// }

static int hash(const candy_wrap_t *key) {
  switch (candy_wrap_get_type(key)) {
    case TYPE_INTEGER:
      return (int)candy_wrap_get_integer(key);
    case TYPE_CHAR:
      return 0;
    default:
      return 0;
  }
}

static inline candy_pair_t *main_position(const candy_table_t *self, const candy_wrap_t *key) {
  return _head(self) + hash(key) % _size(self);
}

static bool equal(const candy_wrap_t *keyl, const candy_wrap_t *keyr) {
  if (keyl->type != keyr->type)
    return false;
  switch (candy_wrap_get_type(keyl)) {
    case TYPE_INTEGER:
      return candy_wrap_get_integer(keyl) == candy_wrap_get_integer(keyr);
    case TYPE_FLOAT:
      return candy_wrap_get_float(keyl) == candy_wrap_get_float(keyr);
    default:
      return false;
  }
}

int expand(const candy_table_t *self) {
  /* todo: expand */
  return 0;
}

int candy_table_fprint(const candy_table_t *self, FILE *out) {
  fprintf(out, "\033[1;35m>>> table %p head\033[0m\n", self);
  fprintf(out, "pos  key-type         key-val  val-type         val-val\n");
  for (candy_pair_t *pair = _head(self); pair <= _tail(self); ++pair) {
    fprintf(out, "%3ld", pair - _head(self));
    fprintf(out, "%10s", candy_wrap_get_typestr(&pair->key));
    candy_wrap_fprint(&pair->key, out, 16);
    fprintf(out, "%10s", candy_wrap_get_typestr(&pair->val));
    candy_wrap_fprint(&pair->val, out, 16);
    fprintf(out, "\n");
  }
  fprintf(out, "\033[1;35m<<< table %p tail\033[0m", self);
  return 0;
}

const candy_wrap_t *candy_table_get(const candy_table_t *self, const candy_wrap_t *key) {
  candy_pair_t *pair = main_position(self, key);
  for (int32_t next = 0; _boundary_check(self, pair + next); next += _get_next(next)) {
    if (equal(&(pair + next)->key, key))
      return &(pair + next)->val;
    else if (candy_wrap_get_type(&(pair + next)->key) == TYPE_NULL)
      break;
  }
  return &null;
}

int candy_table_set(const candy_table_t *self, const candy_wrap_t *key, const candy_wrap_t *val) {
  candy_pair_t *pair = NULL;
  start:
  pair = main_position(self, key);
  for (int32_t next = 0; _boundary_check(self, pair + next); next += _get_next(next)) {
    switch (candy_wrap_get_type(&(pair + next)->key)) {
      case TYPE_NULL:
        (pair + next)->key = *key;
        (pair + next)->val = *val;
        return 0;
      default:
        if (!equal(&(pair + next)->key, key))
          break;
        (pair + next)->val = *val;
        return 0;
    }
  }
  expand(self);
  goto start;
}

candy_table_t *candy_table_create(candy_gc_t *gc) {
  candy_table_t *self = (candy_table_t *)candy_gc_add(gc, TYPE_TABLE, sizeof(struct candy_table));
  return self;
}

int candy_table_delete(candy_table_t *self, candy_gc_t *gc) {

  return 0;
}
