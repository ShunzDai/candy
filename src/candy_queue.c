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
#include "src/candy_queue.h"
#include <stdlib.h>
#include <assert.h>

struct priv {
  uint16_t size;
};

static inline struct priv *_private(candy_queue_t *self) {
  return (struct priv *)((struct {candy_node_t *next; uint8_t args[];} *)self)->args;
}

bool candy_queue_empty(candy_queue_t *self) {
  assert(self);
  return (self->head == NULL);
}

uint32_t candy_queue_size(candy_queue_t *self) {
  assert(self);
  return _private(self)->size;
}

int candy_queue_iterator(candy_queue_t *self, candy_iterator_t func, void *args) {
  assert(self);
  assert(func);
  for (candy_node_t **node = &self->head; *node; node = &(*node)->next) {
    if (func(node, args) != 0)
      break;
  }
  return 0;
}

candy_node_t **candy_queue_pointer(candy_queue_t *self, int32_t idx) {
  assert(self);
  if (idx < 0)
    idx = _private(self)->size + idx + 1;
  assert(idx >= 0 && idx <= _private(self)->size);
  candy_node_t **node = &self->head;
  for (; *node && idx; node = &(*node)->next, idx--);
  return node;
}

int candy_queue_clear(candy_queue_t *self, candy_destroy_t func) {
  assert(self);
  while (!candy_queue_empty(self))
    candy_dequeue(self, 0, func);
  return 0;
}

candy_queue_t *candy_queue_create(void) {
  candy_queue_t *queue = (candy_queue_t *)malloc(sizeof(struct candy_queue) + sizeof(struct priv));
  queue->head = NULL;
  _private(queue)->size = 0;
  return queue;
}

int candy_queue_delete(candy_queue_t **self, candy_destroy_t func) {
  candy_queue_clear(*self, func);
  free(*self);
  *self = NULL;
  return 0;
}

int candy_enqueue(candy_queue_t *self, int32_t idx, candy_node_t *node) {
  assert(self);
  assert(node);
  candy_node_t **temp = candy_queue_pointer(self, idx);
  node->next = *temp;
  *temp = node;
  _private(self)->size++;
  return 0;
}

int candy_dequeue(candy_queue_t *self, int32_t idx, candy_destroy_t func) {
  assert(self);
  candy_node_t **del = candy_queue_pointer(self, idx);
  candy_node_t *next = (*del)->next;
  if (func)
    func(del);
  *del = next;
  _private(self)->size--;
  return 0;
}
