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
#ifndef CANDY_SRC_STRUCT_OBJECT_H
#define CANDY_SRC_STRUCT_OBJECT_H
#ifdef __cplusplus
extern "C"{
#endif /* __cplusplus */

#include "src/common/candy_types.h"
#include "src/struct/candy_queue.h"

candy_object_t candy_object_create(void);
candy_object_t candy_object_delete(candy_object_t obj);

int candy_object_push(candy_object_t obj, candy_node_t node);
#define candy_object_push_none(obj, name) candy_object_push(obj, candy_create_none(name))
#define candy_object_push_string(obj, name, str) candy_object_push(obj, candy_create_string(name, str))
#define candy_object_push_integer(obj, name, value) candy_object_push(obj, candy_create_integer(name, value))
#define candy_object_push_float(obj, name, value) candy_object_push(obj, candy_create_float(name, value))
#define candy_object_push_method(obj, name, method) candy_object_push(obj, candy_create_method(name, method))
int candy_object_pop(candy_object_t obj, char *name);

candy_method_t candy_object_get_method(candy_object_t obj, char *name);

#ifdef __cplusplus
}
#endif /* __cplusplus */
#endif /* CANDY_SRC_STRUCT_OBJECT_H */
