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
#ifndef CANDY_SRC_STRUCT_QUEUE_H
#define CANDY_SRC_STRUCT_QUEUE_H
#ifdef __cplusplus
extern "C"{
#endif /* __cplusplus */

#include "src/common/candy_types.h"
#include "src/struct/candy_node.h"

struct candy_queue;

typedef struct candy_queue * candy_queue_t;

candy_queue_t candy_queue_create(void);
candy_queue_t candy_queue_delete(candy_queue_t queue);

void candy_queue_print(candy_queue_t queue);
int candy_queue_empty(candy_queue_t queue);
int candy_queue_clear(candy_queue_t queue);
uint32_t candy_queue_count(candy_queue_t queue);
candy_node_t *candy_queue_pointer(candy_queue_t queue, uint32_t pos);
candy_node_t *candy_queue_search(candy_queue_t queue, char *name);

int candy_enqueue(candy_queue_t queue, candy_node_t node);
int candy_dequeue_bypos(candy_queue_t queue, uint32_t pos);
int candy_dequeue_byname(candy_queue_t queue, char *name);

#ifdef __cplusplus
}
#endif /* __cplusplus */
#endif /* CANDY_SRC_STRUCT_QUEUE_H */
