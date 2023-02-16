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
#include "src/candy_node.h"
#include <stdlib.h>
#include <string.h>
#include <assert.h>

struct candy_node {
  candy_node_t *next;
};

bool candy_node_empty(candy_node_t *self) {
  return self->next == NULL;
}

int candy_node_iterator(candy_node_t *self, candy_iterator_t func, void *args) {
  for (candy_node_t **node = &self->next; *node; node = &(*node)->next) {
    if (func(node, args) != 0)
      break;
  }
  return 0;
}

int candy_node_clear(candy_node_t *self, candy_destroy_t func) {
  while (!candy_node_empty(self))
    candy_node_delete(&self->next, func);
  return 0;
}

candy_node_t *candy_node_create(int size) {
  assert(sizeof(struct candy_node) < (size_t)size);
  candy_node_t *self = (candy_node_t *)malloc(size);
  memset(self, 0, size);
  return self;
}
int candy_node_delete(candy_node_t **self, candy_destroy_t func) {
  candy_node_t *next = (*self)->next;
  if (func)
    func(self);
  free(*self);
  *self = next;
  return 0;
}

int candy_node_add(candy_node_t *self, candy_node_t *node) {
  candy_node_t **temp = &self->next;
  node->next = *temp;
  *temp = node;
  return 0;
}
