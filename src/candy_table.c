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

struct candy_table {
  candy_node_t *head;
  size_t size;
};

static inline size_t _size(candy_table_t *self) {
  return self->size;
}

static inline candy_node_t *_head(candy_table_t *self) {
  return self->head;
}

static inline candy_node_t *_tail(candy_table_t *self) {
  return _head(self) + _size(self) - 1;
}

static inline bool _boundary_check(candy_table_t *self, candy_node_t *node) {
  return (size_t)(node - _head(self)) <= (_size(self) - 1);
}

static inline int32_t _get_next(int32_t now) {
  return ((now <= 0) - now) << 1;
}

static void _wrap_sprint(candy_table_t *self, const candy_wrap_t *wrap, char buff[], int(*print)(candy_table_t *self, int depth), int depth) {
  switch (wrap->type) {
    case TYPE_NULL: case TYPE_DEL: case TYPE_NONE:
      sprintf(buff, "%16s", "NA");
      break;
    case TYPE_INTEGER:
      sprintf(buff, "%16lld", *candy_wrap_get_integer(wrap));
      break;
    case TYPE_FLOAT:
      sprintf(buff, "%16f", *candy_wrap_get_float(wrap));
      break;
    case TYPE_STRING:
      sprintf(buff, "%16s", candy_wrap_get_string(wrap));
      break;
    case TYPE_USERDEF:
      sprintf(buff, "%16p", *candy_wrap_get_ud(wrap));
      break;
    case TYPE_BUILTIN:
      sprintf(buff, "%16p", *candy_wrap_get_builtin(wrap));
      break;
    case TYPE_TABLE:
      print(self, depth);
      break;
    default:
      break;
  }
}

static int _dump(candy_table_t *self, int depth) {
  static const char *type[] = {
#define CANDY_TYPE_STR
#include "src/candy_type.list"
  };
  printf("%*s\033[1;35m>>> table head\033[0m\n", depth * 2, "");
  printf("%*sdepth  pos  key-type         key-val  val-type         val-val\n", depth * 2, "");
  for (candy_node_t *node = _head(self); node <= _tail(self); ++node) {
    char key[17], val[17];
    _wrap_sprint(self, &node->key, key, _dump, depth);
    _wrap_sprint(self, &node->val, val, _dump, depth);
    printf("%*s%5d%5ld%10s%s%10s%s\n", depth * 2, "", depth, node - _head(self), type[node->key.type], key, type[node->val.type], val);
  }
  printf("%*s\033[1;35m<<< table tail\033[0m\n", depth * 2, "");
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

static inline candy_node_t *main_position(candy_table_t *self, const candy_wrap_t *key) {
  return _head(self) + hash(key) % _size(self);
}

static bool equal(const candy_wrap_t *keyl, const candy_wrap_t *keyr) {
  if (keyl->mask != keyr->mask)
    return false;
  return memcmp(candy_wrap_get_data(keyl), candy_wrap_get_data(keyr), keyl->size * candy_wrap_sizeof(keyl)) == 0;
}

candy_table_t *candy_table_create() {
  candy_table_t *self = calloc(1, sizeof(struct candy_table));
  self->head = calloc(16, sizeof(struct candy_node));
  self->size = 16;
  return self;
}

int candy_table_delete(candy_table_t **self) {
  free((*self)->head);
  (*self)->head = NULL;
  free(*self);
  *self = NULL;
  return 0;
}

int candy_table_dump(candy_table_t *self) {
  return _dump(self, 0);
}

const candy_wrap_t *candy_table_get(candy_table_t *self, const candy_wrap_t *key) {
  candy_node_t *node = main_position(self, key);
  for (int32_t next = 0; _boundary_check(self, node + next); next += _get_next(next)) {
    if (equal(&(node + next)->key, key))
      return &(node + next)->val;
    else if ((node + next)->key.type == TYPE_NULL)
      break;
  }
  return &null;
}

int candy_table_set(candy_table_t *self, const candy_wrap_t *key, const candy_wrap_t *val) {
  candy_node_t *node = main_position(self, key);
  for (int32_t next = 0; _boundary_check(self, node + next); next += _get_next(next)) {
    switch ((node + next)->key.type) {
      case TYPE_NULL: case TYPE_DEL:
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
