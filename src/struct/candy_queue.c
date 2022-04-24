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
#include "src/struct/candy_object.h"

#define _private(queue) ((struct queue_private *)((queue)->priv))

struct candy_queue{
  candy_node_t next;
  uint8_t priv[];
};

struct queue_private{
  uint8_t count;
};

candy_node_t candy_node_delete(candy_node_t node){
  candy_node_t temp = NULL;
  uint8_t *buffer = NULL;
  if (node != NULL){
    temp = node->next;
    switch (candy_node_get_type(node)){
      case CANDY_TYPES_OBJECT:
        buffer = candy_pack_get_buffer((candy_pack_t)node);
        buffer = (uint8_t *)candy_queue_delete(*(candy_queue_t *)buffer);
        break;
      default:
        break;
    }
    candy_free(node);
  }
  return temp;
}

candy_queue_t candy_queue_create(void){
  candy_queue_t queue = (candy_queue_t)candy_malloc(sizeof(struct candy_queue) + sizeof(struct queue_private));
  queue->next = NULL;
  _private(queue)->count = 0;
  return queue;
}

candy_queue_t candy_queue_delete(candy_queue_t queue){
  if (queue != NULL){
    candy_queue_clear(queue);
    candy_free(queue);
  }
  return NULL;
}

candy_node_t *candy_queue_pointer(candy_queue_t queue, int32_t pos){
  candy_assert(queue != NULL);
  candy_node_t *temp = &queue->next;
  if (pos < 0)
    pos = candy_queue_count(queue) ? candy_queue_count(queue) + pos : 0;
  while (temp != NULL && pos--){
    candy_assert(*temp != NULL);
    temp = candy_node_get_next(*temp);
  }
  return temp;
}

uint32_t candy_queue_count(candy_queue_t queue){
  candy_assert(queue != NULL);
  return _private(queue)->count;
}

int candy_queue_empty(candy_queue_t queue){
  candy_assert(queue != NULL);
  return (queue->next == NULL);
}

int candy_queue_clear(candy_queue_t queue){
  candy_assert(queue != NULL);
  while (!candy_queue_empty(queue))
    candy_dequeue(queue, 0);
  return 1;
}

int candy_enqueue(candy_queue_t queue, int32_t pos, candy_node_t node){
  candy_assert(queue != NULL);
  candy_assert(node != NULL);
  candy_node_t *temp = (candy_queue_count(queue) && pos) ? candy_node_get_next(*candy_queue_pointer(queue, pos)) : &queue->next;
  node->next = *temp;
  *temp = node;
  _private(queue)->count++;
  return 1;
}

int candy_dequeue(candy_queue_t queue, int32_t pos){
  candy_assert(queue != NULL);
  candy_node_t *temp = candy_queue_pointer(queue, pos);
  *temp = candy_node_delete(*temp);
  _private(queue)->count--;
  return 1;
}
