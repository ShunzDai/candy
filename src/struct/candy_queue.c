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

candy_queue_t candy_queue_delete(candy_queue_t queue){
  if (queue != NULL){
    candy_queue_clear(queue);
    candy_free(queue);
  }
  return NULL;
}

void candy_queue_print(candy_queue_t queue){
  candy_assert(queue != NULL);
  candy_node_t *temp = &queue->next;
  uint32_t count = 0;
  printf("\033[1;35m>>>\tqueue head\033[0m\n");
  while (1){
    if (temp == NULL)
      break;
    else if (*temp == NULL)
      break;
    printf("%d\t%p\n", count, temp);
    count++;
    temp = candy_node_get_next(*temp);
  }
  printf("\033[1;35m>>>\tqueue tail\033[0m\n");
}

int candy_queue_empty(candy_queue_t queue){
  candy_assert(queue != NULL);
  return (queue->next == NULL);
}

int candy_queue_clear(candy_queue_t queue){
  candy_assert(queue != NULL);
  while (!candy_queue_empty(queue))
    candy_dequeue_bypos(queue, 0);
  return 1;
}

uint32_t candy_queue_count(candy_queue_t queue){
  candy_assert(queue != NULL);
  candy_node_t *temp = &queue->next;
  uint32_t count = 0;
  while (1){
    if (temp == NULL)
      break;
    else if (*temp == NULL)
      break;
    count++;
    temp = candy_node_get_next(*temp);
  }
  return count;
}

candy_node_t *candy_queue_pointer(candy_queue_t queue, uint32_t pos){
  candy_assert(queue != NULL);
  candy_node_t *temp = &queue->next;
  while (pos--){
    if (temp == NULL)
      return 0;
    else if (*temp == NULL)
      return 0;
    temp = candy_node_get_next(*temp);
  }
  return temp;
}

candy_node_t *candy_queue_search(candy_queue_t queue, char *name){
  candy_assert(queue != NULL);
  candy_node_t *temp = &queue->next;
  while (temp != NULL){
    if (candy_node_checkout(*temp, name))
      break;
    temp = candy_node_get_next(*temp);
  }
  return temp;
}

int candy_enqueue(candy_queue_t queue, candy_node_t node){
  candy_assert(queue != NULL);
  candy_assert(node != NULL);
  uint32_t count = candy_queue_count(queue);
  count = count ? count - 1 : count;
  candy_node_t *temp = candy_queue_pointer(queue, count);
  *candy_node_get_next(node) = *temp;
  *temp = node;
  return 1;
}

int candy_dequeue_bypos(candy_queue_t queue, uint32_t pos){
  candy_assert(queue != NULL);
  candy_node_t *temp = candy_queue_pointer(queue, pos);
  *temp = candy_node_delete(*temp);
  return 1;
}

int candy_dequeue_byname(candy_queue_t queue, char *name){
  candy_assert(queue != NULL);
  candy_node_t *temp = candy_queue_search(queue, name);
  *temp = candy_node_delete(*temp);
  return 1;
}
