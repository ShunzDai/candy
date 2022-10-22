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

#include <stdint.h>
#include <stdbool.h>

struct candy_node;

struct candy_node {
  struct candy_node *next;
};

struct candy_queue;

struct candy_queue {
  struct candy_node *head;
};

/**
  * @brief  free the heap space, for example, free(*node).
  */
typedef int (*candy_destroy_t)(struct candy_node **node);

typedef void (*candy_iterator_t)(struct candy_node **node, void *args);

bool candy_queue_empty(struct candy_queue *queue);
uint32_t candy_queue_size(struct candy_queue *queue);
int candy_queue_iterator(struct candy_queue *queue, candy_iterator_t func, void *args);
struct candy_node **candy_queue_pointer(struct candy_queue *queue, int32_t idx);
int candy_queue_clear(struct candy_queue *queue, candy_destroy_t func);

struct candy_queue *candy_queue_create(void);
int candy_queue_delete(struct candy_queue **queue, candy_destroy_t func);

int candy_enqueue(struct candy_queue *queue, int32_t idx, struct candy_node *node);
int candy_dequeue(struct candy_queue *queue, int32_t idx, candy_destroy_t func);

#ifdef __cplusplus
}
#endif /* __cplusplus */
#endif /* CANDY_SRC_STRUCT_QUEUE_H */
