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
#ifndef CANDY_SRC_STRUCT_NODE_H
#define CANDY_SRC_STRUCT_NODE_H
#ifdef __cplusplus
extern "C"{
#endif /* __cplusplus */

#include "src/common/candy_types.h"
#include "src/struct/candy_builtin.h"

struct candy_node;

typedef struct candy_node * candy_node_t;

typedef uint32_t candy_hash_t;

candy_node_t candy_node_create(char *name);
candy_node_t candy_node_delete(candy_node_t node);

int candy_node_checkout(candy_node_t node, char *str);

candy_node_t candy_node_set_none(candy_node_t node);

char *candy_node_get_string(candy_node_t node);
candy_node_t candy_node_set_string(candy_node_t node, char *str);

int candy_node_get_integer(candy_node_t node);
candy_node_t candy_node_set_integer(candy_node_t node, int value);

float candy_node_get_float(candy_node_t node);
candy_node_t candy_node_set_float(candy_node_t node, float value);

#ifdef __cplusplus
}
#endif /* __cplusplus */
#endif /* CANDY_SRC_STRUCT_NODE_H */
