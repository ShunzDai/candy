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
#include "src/platform/candy_memory.h"

typedef struct priv{
  uint16_t count;
} * priv_t;

static inline priv_t _private(candy_queue_t queue){
  return (priv_t)(((struct {candy_node_t next; uint8_t args[];} *)queue)->args);
}

inline uint16_t candy_queue_size(void){
  return sizeof(struct candy_queue) + sizeof(struct priv);
}

inline bool candy_queue_empty(candy_queue_t queue){
  candy_assert(queue != NULL);
  return (queue->next == NULL);
}

inline uint32_t candy_queue_count(candy_queue_t queue){
  candy_assert(queue != NULL);
  return _private(queue)->count;
}

int candy_queue_iterator(candy_queue_t queue, candy_iterator_t func, void *args){
  candy_assert(queue != NULL);
  for (candy_node_t *node = &queue->next; ;){
    if (*node == NULL)
      break;
    if (func != NULL)
      func(node, args);
    node = &(*node)->next;
  }
  return 0;
}

candy_node_t *candy_queue_pointer(candy_queue_t queue, int32_t pos){
  candy_assert(queue != NULL);
  if (_private(queue)->count == 0)
    return &queue->next;
  else if (pos < 0)
    pos = _private(queue)->count + pos;
  for (candy_node_t *node = &queue->next; ; pos--){
    if (pos == 0)
      return node;
    node = &(*node)->next;
  }
  return &queue->next;
}

int candy_queue_clear(candy_queue_t queue, candy_destroy_t func){
  candy_assert(queue != NULL);
  while (!candy_queue_empty(queue))
    candy_dequeue(queue, 0, func);
  return 0;
}

candy_queue_t candy_queue_create(void){
  candy_queue_t queue = (candy_queue_t)candy_malloc(candy_queue_size());
  queue->next = NULL;
  _private(queue)->count = 0;
  return queue;
}

int candy_queue_delete(candy_queue_t *queue, candy_destroy_t func){
  candy_queue_clear(*queue, func);
  candy_free(*queue);
  *queue = NULL;
  return 0;
}

int candy_enqueue(candy_queue_t queue, int32_t pos, candy_node_t node){
  candy_assert(queue != NULL);
  candy_assert(node != NULL);
  candy_node_t *temp = candy_queue_pointer(queue, pos);
  if (_private(queue)->count && pos != 0)
    temp = &(*temp)->next;
  node->next = *temp;
  *temp = node;
  _private(queue)->count++;
  return 0;
}

int candy_dequeue(candy_queue_t queue, int32_t pos, candy_destroy_t func){
  candy_assert(queue != NULL);
  candy_node_t *temp = candy_queue_pointer(queue, pos);
  candy_node_t del = *temp;
  *temp = (*temp)->next;
  if (func)
    func(&del);
  _private(queue)->count--;
  return 0;
}
