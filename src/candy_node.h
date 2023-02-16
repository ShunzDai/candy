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
#ifndef CANDY_SRC_NODE_H
#define CANDY_SRC_NODE_H
#ifdef __cplusplus
extern "C"{
#endif /* __cplusplus */

#include "src/candy_types.h"

/**
  * @brief  destroy payload, do not operate node
  */
typedef int (*candy_destroy_t)(candy_node_t **);

typedef int (*candy_iterator_t)(candy_node_t **, void *);

bool candy_node_empty(candy_node_t *self);
int candy_node_iterator(candy_node_t *self, candy_iterator_t func, void *args);
int candy_node_clear(candy_node_t *self, candy_destroy_t func);
int candy_node_add(candy_node_t *self, candy_node_t *node);

candy_node_t *candy_node_create(int size);
int candy_node_delete(candy_node_t **self, candy_destroy_t func);

#ifdef __cplusplus
}
#endif /* __cplusplus */
#endif /* CANDY_SRC_NODE_H */
