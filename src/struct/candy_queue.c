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

struct candy_queue{
  candy_node_t next;
};

candy_queue_t candy_queue_create(void){
  candy_queue_t queue = (candy_queue_t)candy_malloc(sizeof(struct candy_queue));
  queue->next = NULL;
  return queue;
}

candy_queue_t candy_queue_delete(candy_queue_t queue, candy_destruct_t func){
  if (queue != NULL){
    candy_queue_clear(queue, func);
    candy_free(queue);
  }
  return NULL;
}

int candy_queue_empty(candy_queue_t queue){
  candy_assert(queue != NULL, "");
  return (queue->next == NULL);
}

int candy_queue_clear(candy_queue_t queue, candy_destruct_t func){
  candy_assert(queue != NULL, "");
  while (!candy_queue_empty(queue))
    candy_dequeue(queue, 0, func);
  return 1;
}

uint32_t candy_queue_count(candy_queue_t queue){
  candy_assert(queue != NULL, "");
  candy_node_t *temp = &queue->next;
  uint32_t count = 0;
  while (temp != NULL){
    if (*temp == NULL)
      break;
    temp = candy_get_next(*temp);
    count++;
  }
  return count;
}

candy_node_t *candy_queue_pointer(candy_queue_t queue, int32_t pos){
  candy_assert(queue != NULL, "");
  candy_node_t *temp = &queue->next;
  candy_node_t *next = NULL;
  while (temp != NULL && pos--){
    if (*temp == NULL)
      return temp;
    next = candy_get_next(*temp);
    if (*next == NULL){
      candy_assert(pos < 0, "pos beyond border");
      return temp;
    }
    temp = next;
  }
  return temp;
}

int candy_enqueue(candy_queue_t queue, int32_t pos, candy_node_t node){
  candy_assert(queue != NULL, "");
  candy_assert(node != NULL, "");
  candy_node_t *temp = candy_queue_pointer(queue, pos);
  temp = (pos < 0 && *temp != NULL) ? candy_get_next(*temp) : temp;
  *candy_get_next(node) = *temp;
  *temp = node;
  return 1;
}

int candy_dequeue(candy_queue_t queue, int32_t pos, candy_destruct_t func){
  candy_assert(queue != NULL, "");
  candy_node_t *temp = candy_queue_pointer(queue, pos);
  *temp = func(*temp);
  return 1;
}
