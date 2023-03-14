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
#include "src/candy_object.h"
#include "src/candy_node.h"
#include "src/candy_lib.h"
#include "candy_wrap.h"

struct candy_object {
  candy_object_t *next;
  uint32_t hash;
  candy_wrap_t wrap;
};

struct iterator {
  const uint32_t hash;
  candy_wrap_t *wrap;
};

static const char *_wrap_type[] = {
  "none", "integer", "float", "boolean",
  "string", "ud", "builtin", "object",
  "unknown",
};

static int _destroy(candy_object_t **self) {
  /* clear branch */
  if ((*self)->wrap.type == CANDY_OBJECT) {
    candy_node_clear(*(candy_node_t **)candy_wrap_get_object(&(*self)->wrap), (candy_destroy_t)_destroy);
    candy_node_delete((candy_node_t **)candy_wrap_get_object(&(*self)->wrap), (candy_destroy_t)_destroy);
  }
  return candy_wrap_deinit(&(*self)->wrap);
}

static void _print(candy_object_t *self, int depth) {
  uint32_t count = 0;
  char offset[255];
  memset(offset, '\t', depth);
  offset[depth] = '\0';
  printf("%s\033[1;35m>>> %sobject head\033[0m\n", offset, depth ? "sub " : "");
  printf("%sdepth-pos\taddress\t\ttype\tsize\thash\t\tvalue\n", offset);
  while (self != NULL) {
    printf("%s[%d-%d]\t\t%p\t%s\t%d\t0x%08X\t", offset, depth, count, self, _wrap_type[self->wrap.type], self->wrap.size, self->hash);
    switch (self->wrap.type) {
      case CANDY_NONE:    printf("NA\n");                                                       break;
      case CANDY_INTEGER: printf("%ld\n", *candy_wrap_get_integer(&self->wrap));                break;
      case CANDY_FLOAT:   printf("%.3f\n", *candy_wrap_get_float(&self->wrap));                 break;
      case CANDY_BOOLEAN: printf("%d\n", *candy_wrap_get_boolean(&self->wrap));                 break;
      case CANDY_STRING:  printf("%s\n", candy_wrap_get_string(&self->wrap));                   break;
      case CANDY_USERDEF: printf("%p\n", *candy_wrap_get_ud(&self->wrap));                      break;
      case CANDY_BUILTIN: printf("%p\n", *candy_wrap_get_builtin(&self->wrap));                 break;
      case CANDY_OBJECT:  printf("\n"); _print(*candy_wrap_get_object(&self->wrap), depth + 1); break;
      default: printf("NA\n"); break;
    }
    count++;
    self = self->next;
  }
  printf("%s\033[1;35m<<< %sobject tail\033[0m\n", offset, depth ? "sub " : "");
}

static int _iterator(candy_object_t **self, struct iterator *it) {
  if ((*self)->hash != it->hash)
    return -1;
  return it->wrap = &(*self)->wrap, 0;
}

candy_object_t *candy_object_create(const char name[]) {
  candy_object_t *self = (candy_object_t *)candy_node_create(sizeof(struct candy_object));
  self->hash = djb_hash(name);
  return self;
}

int candy_object_delete(candy_object_t **self) {
  return candy_node_delete((candy_node_t **)self, (candy_destroy_t)_destroy);
}

uint32_t candy_object_hash(candy_object_t *self) {
  return self->hash;
}

void candy_object_print(candy_object_t *self) {
  _print(self, 0);
}

candy_wrap_t *candy_object_find_wrap(candy_object_t *self, const char name[]) {
  struct iterator it = {
    .hash = djb_hash(name),
    .wrap = NULL,
  };
  if (self->wrap.type == CANDY_OBJECT)
    candy_node_iterator(*(candy_node_t **)candy_wrap_get_object(&self->wrap), (candy_iterator_t)_iterator, &it);
  return it.wrap;
}

int candy_object_add_wrap(candy_object_t *self, const char name[], candy_wrap_t *wrap) {
  candy_object_t *obj = candy_object_create(name);
  obj->wrap = *wrap;
  return candy_object_add_object(self, obj);
}

int candy_object_add_integer(candy_object_t *self, const char name[], const candy_integer_t val) {
  candy_wrap_t wrap;
  candy_wrap_init_integer(&wrap, &val, 1);
  return candy_object_add_wrap(self, name, &wrap);
}

int candy_object_add_float(candy_object_t *self, const char name[], const candy_float_t val) {
  candy_wrap_t wrap;
  candy_wrap_init_float(&wrap, &val, 1);
  return candy_object_add_wrap(self, name, &wrap);
}

int candy_object_add_boolean(candy_object_t *self, const char name[], const candy_boolean_t val) {
  candy_wrap_t wrap;
  candy_wrap_init_boolean(&wrap, &val, 1);
  return candy_object_add_wrap(self, name, &wrap);
}

int candy_object_add_string(candy_object_t *self, const char name[], const char val[], size_t size) {
  candy_wrap_t wrap;
  candy_wrap_init_string(&wrap, val, size);
  return candy_object_add_wrap(self, name, &wrap);
}

int candy_object_add_ud(candy_object_t *self, const char name[], const void *val) {
  candy_wrap_t wrap;
  candy_wrap_init_ud(&wrap, &val, 1);
  return candy_object_add_wrap(self, name, &wrap);
}

int candy_object_add_builtin(candy_object_t *self, const char name[], const candy_builtin_t val) {
  candy_wrap_t wrap;
  candy_wrap_init_builtin(&wrap, &val, 1);
  return candy_object_add_wrap(self, name, &wrap);
}

int candy_object_add_object(candy_object_t *self, const candy_object_t *val) {
  assert(val->next == NULL);
  if (self->wrap.type != CANDY_NONE)
    *(candy_object_t **)&val->next = *candy_wrap_get_object(&self->wrap);
  return candy_wrap_init_object(&self->wrap, &val, 1), 0;
}
