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

typedef enum candy_builtin{
  CANDY_BUILTIN_NONE,
  CANDY_BUILTIN_STRING,
  CANDY_BUILTIN_INTEGER,
  CANDY_BUILTIN_FLOAT,
  CANDY_BUILTIN_METHOD,
} candy_builtin_t;

candy_node_t candy_create_none(char *name);
candy_node_t candy_create_string(char *name, candy_string_t *str);
candy_node_t candy_create_integer(char *name, candy_integer_t value);
candy_node_t candy_create_float(char *name, candy_float_t value);
candy_node_t candy_create_method(char *name, candy_method_t method);
candy_node_t candy_node_delete(candy_node_t node);

int candy_node_checkout(candy_node_t node, char *name);
candy_node_t *candy_node_get_next(candy_node_t node);

candy_node_t candy_node_set_none(candy_node_t node);
candy_node_t candy_node_set_string(candy_node_t node, candy_string_t *str);
candy_node_t candy_node_set_integer(candy_node_t node, candy_integer_t value);
candy_node_t candy_node_set_float(candy_node_t node, candy_float_t value);
candy_node_t candy_node_set_method(candy_node_t node, candy_method_t method);

candy_string_t *candy_node_get_string(candy_node_t node);
candy_integer_t candy_node_get_integer(candy_node_t node);
candy_float_t candy_node_get_float(candy_node_t node);
candy_method_t candy_node_get_method(candy_node_t node);

#ifdef __cplusplus
}
#endif /* __cplusplus */
#endif /* CANDY_SRC_STRUCT_NODE_H */
