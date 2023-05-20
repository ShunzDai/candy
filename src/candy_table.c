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

typedef struct candy_node candy_node_t;

struct candy_node {
  candy_wrap_t key;
  candy_wrap_t val;
};

static inline size_t _size(const candy_wrap_t *self) {
  return self->size;
}

static inline candy_node_t *_head(const candy_wrap_t *self) {
  return (candy_node_t *)candy_wrap_get_table(self);
}

static inline candy_node_t *_tail(const candy_wrap_t *self) {
  return _head(self) + _size(self) - 1;
}

static inline bool _boundary_check(candy_wrap_t *self, candy_node_t *node) {
  return (size_t)(node - _head(self)) <= (_size(self) - 1);
}

static inline int32_t _get_next(int32_t now) {
  return ((now <= 0) - now) << 1;
}

static void _wrap_sprint(const candy_wrap_t *wrap, FILE *out, int(*print)(const candy_wrap_t *, FILE *, int), int depth) {
  switch (wrap->type) {
    case TYPE_NULL:
      fprintf(out, "%10s", "NA");
      break;
    case TYPE_INTEGER:
      fprintf(out, "%10lld", *candy_wrap_get_integer(wrap));
      break;
    case TYPE_FLOAT:
      fprintf(out, "%10f", *candy_wrap_get_float(wrap));
      break;
    case TYPE_STRING:
      fprintf(out, "%10.*s", wrap->size, candy_wrap_get_string(wrap));
      break;
    case TYPE_USERDEF:
      fprintf(out, "%10p", *candy_wrap_get_ud(wrap));
      break;
    case TYPE_BUILTIN:
      fprintf(out, "%10p", *candy_wrap_get_builtin(wrap));
      break;
    case TYPE_TABLE:
      fprintf(out, "%10p\n", wrap);
      print(wrap, out, depth + 1);
      break;
    default:
      break;
  }
}

static int _dump(const candy_wrap_t *self, FILE *out, int depth) {
  static const char *type[] = {
#define CANDY_TYPE_STR
#include "src/candy_type.list"
  };
  fprintf(out, "%*s\033[1;35m>>> depth %d, table %p head\033[0m\n", depth * 2, "", depth, self);
  fprintf(out, "%*spos  key-type   key-val  val-type   val-val\n", depth * 2, "");
  for (candy_node_t *node = _head(self); node <= _tail(self); ++node) {
    fprintf(out, "%*s%3ld", depth * 2, "", node - _head(self));
    fprintf(out, "%10s", type[node->key.type]);
    _wrap_sprint(&node->key, out, _dump, depth);
    fprintf(out, "%10s", type[node->val.type]);
    _wrap_sprint(&node->val, out, _dump, depth);
    fprintf(out, "\n");
  }
  fprintf(out, "%*s\033[1;35m<<< depth %d, table %p tail\033[0m%s", depth * 2, "", depth, self, depth ? "" : "\n");
  return 0;
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
    case TYPE_STRING:
      return hash_string(candy_wrap_get_string(key), key->size);
    default:
      return 0;
  }
}

static inline candy_node_t *main_position(candy_wrap_t *self, const candy_wrap_t *key) {
  return _head(self) + hash(key) % _size(self);
}

static bool equal(const candy_wrap_t *keyl, const candy_wrap_t *keyr) {
  if (keyl->mask != keyr->mask)
    return false;
  return memcmp(candy_wrap_get_data(keyl), candy_wrap_get_data(keyr), keyl->size * candy_wrap_sizeof(keyl)) == 0;
}

int candy_table_dump(const candy_wrap_t *self, FILE *out) {
  return _dump(self, out, 0);
}

const candy_wrap_t *candy_table_get(candy_wrap_t *self, const candy_wrap_t *key) {
  candy_node_t *node = main_position(self, key);
  for (int32_t next = 0; _boundary_check(self, node + next); next += _get_next(next)) {
    if (equal(&(node + next)->key, key))
      return &(node + next)->val;
    else if ((node + next)->key.type == TYPE_NULL)
      break;
  }
  return &null;
}

int candy_table_set(candy_wrap_t *self, const candy_wrap_t *key, const candy_wrap_t *val) {
  candy_node_t *node = main_position(self, key);
  for (int32_t next = 0; _boundary_check(self, node + next); next += _get_next(next)) {
    switch ((node + next)->key.type) {
      case TYPE_NULL:
        (node + next)->key = *key;
        (node + next)->val = *val;
        return 0;
      default:
        if (equal(&(node + next)->key, key)) {
          (node + next)->val = *val;
          return 0;
        }
        break;
    }
  }
  return -1;
}
