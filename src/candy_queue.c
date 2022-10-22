/**
  * Copyright 2022 ShunzDai
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
#include "candy_queue.h"
#include <stdlib.h>
#include <assert.h>

struct priv {
  uint16_t size;
};

static inline struct priv *_private(struct candy_queue *queue) {
  return (struct priv *)((struct {struct candy_node *next; uint8_t args[];} *)queue)->args;
}

bool candy_queue_empty(struct candy_queue *queue) {
  assert(queue);
  return (queue->head == NULL);
}

uint32_t candy_queue_size(struct candy_queue *queue) {
  assert(queue);
  return _private(queue)->size;
}

int candy_queue_iterator(struct candy_queue *queue, candy_iterator_t func, void *args) {
  assert(queue);
  assert(func);
  for (struct candy_node **node = &queue->head; *node; node = &(*node)->next) {
    func(node, args);
  }
  return 0;
}

struct candy_node **candy_queue_pointer(struct candy_queue *queue, int32_t idx) {
  assert(queue);
  if (idx < 0)
    idx = _private(queue)->size + idx + 1;
  assert(idx >= 0 && idx <= _private(queue)->size);
  struct candy_node **node = &queue->head;
  for (; *node && idx; node = &(*node)->next, idx--);
  return node;
}

int candy_queue_clear(struct candy_queue *queue, candy_destroy_t func) {
  assert(queue);
  while (!candy_queue_empty(queue))
    candy_dequeue(queue, 0, func);
  return 0;
}

struct candy_queue *candy_queue_create(void) {
  struct candy_queue *queue = (struct candy_queue *)malloc(sizeof(struct candy_queue) + sizeof(struct priv));
  queue->head = NULL;
  _private(queue)->size = 0;
  return queue;
}

int candy_queue_delete(struct candy_queue **queue, candy_destroy_t func) {
  candy_queue_clear(*queue, func);
  free(*queue);
  *queue = NULL;
  return 0;
}

int candy_enqueue(struct candy_queue *queue, int32_t idx, struct candy_node *node) {
  assert(queue);
  assert(node);
  struct candy_node **temp = candy_queue_pointer(queue, idx);
  node->next = *temp;
  *temp = node;
  _private(queue)->size++;
  return 0;
}

int candy_dequeue(struct candy_queue *queue, int32_t idx, candy_destroy_t func) {
  assert(queue);
  struct candy_node **del = candy_queue_pointer(queue, idx);
  struct candy_node *next = (*del)->next;
  if (func)
    func(del);
  *del = next;
  _private(queue)->size--;
  return 0;
}
