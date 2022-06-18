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

struct candy_object{
  candy_object_t next;
};

int candy_object_print(candy_object_t obj);
candy_wrap_t *candy_object_search(candy_object_t obj, candy_hash_t hash);
int candy_object_register(candy_object_t obj, candy_register_t table[]);

candy_object_t candy_object_create(candy_hash_t hash);

int candy_object_delete(candy_object_t *obj);

int candy_object_push(candy_object_t obj, candy_wrap_t wrap);
int candy_object_push_none(candy_object_t obj, candy_hash_t hash);
int candy_object_push_integer(candy_object_t obj, candy_hash_t hash, candy_integer_t value);
int candy_object_push_float(candy_object_t obj, candy_hash_t hash, candy_float_t value);
int candy_object_push_method(candy_object_t obj, candy_hash_t hash, candy_method_t value);
int candy_object_push_string(candy_object_t obj, candy_hash_t hash, const char *value, uint16_t size);
int candy_object_push_object(candy_object_t obj, candy_hash_t hash);

int candy_object_pop(candy_object_t obj, candy_hash_t hash);

candy_string_t candy_object_get_string(candy_object_t obj, candy_hash_t hash);
candy_integer_t candy_object_get_integer(candy_object_t obj, candy_hash_t hash);
candy_float_t candy_object_get_float(candy_object_t obj, candy_hash_t hash);
candy_method_t candy_object_get_method(candy_object_t obj, candy_hash_t hash);
candy_object_t candy_object_get_object(candy_object_t obj, candy_hash_t hash);

#ifdef __cplusplus
}
#endif /* __cplusplus */
#endif /* CANDY_SRC_STRUCT_OBJECT_H */
