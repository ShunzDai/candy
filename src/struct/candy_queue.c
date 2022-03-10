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
  candy_pack_t next;
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
  candy_pack_t *temp = &queue->next;
  uint32_t count = 0;
  uint16_t size = 0;
  printf("\033[1;35m==================== queue head ====================\033[0m\n");
  while (temp != NULL){
    if (*temp == NULL)
      break;
    printf("[%d]\t%p\t", count, *temp);
    switch (candy_pack_get_type(*temp)){
      case CANDY_BUILTIN_NONE:
        printf("none\n");
        break;
      case CANDY_BUILTIN_STRING:
        printf("string\t%s\n", candy_pack_get_string(*temp, &size));
        break;
      case CANDY_BUILTIN_INTEGER:
        printf("integer\t%ld\n", candy_pack_get_integer(*temp));
        break;
      case CANDY_BUILTIN_FLOAT:
        printf("float\t%.5f\n", candy_pack_get_float(*temp));
        break;
      case CANDY_BUILTIN_METHOD:
        printf("method\t%p\n", candy_pack_get_method(*temp));
        break;
      case CANDY_BUILTIN_OBJECT:
        printf("object\t%p\n", candy_pack_get_object(*temp));
        break;
      default:
        printf("exception types\n");
        break;
    }
    count++;
    temp = candy_pack_get_next(*temp);
  }
  printf("pack count(s): %d\n", count);
  printf("\033[1;35m==================== queue tail ====================\033[0m\n");
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
  candy_pack_t *temp = &queue->next;
  uint32_t count = 0;
  while (temp != NULL){
    if (*temp == NULL)
      break;
    temp = candy_pack_get_next(*temp);
    count++;
  }
  return count;
}

candy_pack_t *candy_queue_pointer(candy_queue_t queue, int32_t pos){
  candy_assert(queue != NULL);
  candy_pack_t *temp = &queue->next;
  while (temp != NULL && pos--){
    candy_pack_t *next = candy_pack_get_next(*temp);
    if (*temp == NULL)
      return temp;
    else if (*next == NULL)
      return next;
    temp = next;
  }
  return temp;
}

candy_pack_t *candy_queue_search(candy_queue_t queue, char *name){
  candy_assert(queue != NULL);
  candy_pack_t *temp = &queue->next;
  while (temp != NULL){
    if (*temp == NULL)
      return NULL;
    else if (candy_pack_checkout(*temp, name))
      break;
    temp = candy_pack_get_next(*temp);
  }
  return temp;
}

int candy_enqueue(candy_queue_t queue, candy_pack_t pack, int32_t pos){
  candy_assert(queue != NULL);
  candy_assert(pack != NULL);
  candy_pack_t *temp = candy_queue_pointer(queue, pos);
  candy_assert(temp != NULL);
  *candy_pack_get_next(pack) = *temp;
  *temp = pack;
  return 1;
}

int candy_dequeue_bypos(candy_queue_t queue, int32_t pos){
  candy_assert(queue != NULL);
  candy_pack_t *temp = candy_queue_pointer(queue, pos);
  *temp = candy_pack_delete(*temp);
  return 1;
}

int candy_dequeue_byname(candy_queue_t queue, char *name){
  candy_assert(queue != NULL);
  candy_pack_t *temp = candy_queue_search(queue, name);
  if (temp == NULL)
    return 0;
  *temp = candy_pack_delete(*temp);
  return 1;
}
