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

void candy_object_print(candy_object_t obj);

int candy_object_register(candy_object_t obj, candy_register_t reg[]);

int candy_object_push(candy_object_t obj, candy_pack_t pack);
#define candy_object_push_none(obj, name) candy_object_push(obj, candy_pack_none(name))
#define candy_object_push_string(obj, name, string) candy_object_push(obj, candy_pack_string(name, string))
#define candy_object_push_integer(obj, name, value) candy_object_push(obj, candy_pack_integer(name, value))
#define candy_object_push_float(obj, name, value) candy_object_push(obj, candy_pack_float(name, value))
#define candy_object_push_method(obj, name, method) candy_object_push(obj, candy_pack_method(name, method))
#define candy_object_push_object(obj, name, object) candy_object_push(obj, candy_pack_object(name, object))
int candy_object_pop(candy_object_t obj, char *name);

candy_string_t candy_object_get_string(candy_object_t obj, char *name, uint16_t *size);
candy_integer_t candy_object_get_integer(candy_object_t obj, char *name);
candy_float_t candy_object_get_float(candy_object_t obj, char *name);
candy_method_t candy_object_get_method(candy_object_t obj, char *name);
candy_object_t candy_object_get_object(candy_object_t obj, char *name);

#ifdef __cplusplus
}
#endif /* __cplusplus */
#endif /* CANDY_SRC_STRUCT_OBJECT_H */
