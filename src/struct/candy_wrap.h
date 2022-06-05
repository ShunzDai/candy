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
#ifndef CANDY_SRC_STRUCT_WRAP_H
#define CANDY_SRC_STRUCT_WRAP_H
#ifdef __cplusplus
extern "C"{
#endif /* __cplusplus */

#include "src/common/candy_types.h"

struct candy_wrap{
  candy_wrap_t next;
};

typedef enum candy_wraps{
  CANDY_WRAP_NONE,
  CANDY_WRAP_INTEGER,
  CANDY_WRAP_FLOAT,
  CANDY_WRAP_BOOLEAN,
  CANDY_WRAP_STRING,
  CANDY_WRAP_METHOD,
  CANDY_WRAP_OBJECT,
  CANDY_WRAP_MAX,
} candy_wraps_t;

/* wrap general methods */

candy_span_t candy_wrap_expand(candy_wrap_t wrap);
candy_wraps_t candy_wrap_type(candy_wrap_t wrap);
bool candy_wrap_match(candy_wrap_t wrap, candy_hash_t hash);

/* wrap create methods */

candy_wrap_t candy_wrap_copy(candy_wrap_t wrap);
candy_wrap_t candy_wrap_none(candy_hash_t hash);
candy_wrap_t candy_wrap_integer(candy_hash_t hash, candy_integer_t value);
candy_wrap_t candy_wrap_float(candy_hash_t hash, candy_float_t value);
candy_wrap_t candy_wrap_boolean(candy_hash_t hash, candy_boolean_t value);
candy_wrap_t candy_wrap_method(candy_hash_t hash, candy_method_t value);
candy_wrap_t candy_wrap_string(candy_hash_t hash, candy_string_t value);
candy_wrap_t candy_wrap_meta(candy_hash_t hash, candy_span_t span, candy_wraps_t type);

/* wrap delete method */

candy_wrap_t candy_wrap_delete(candy_wrap_t wrap);

/* wrap set methods */

candy_wrap_t candy_wrap_set_none(candy_wrap_t wrap);
candy_wrap_t candy_wrap_set_integer(candy_wrap_t wrap, candy_integer_t value);
candy_wrap_t candy_wrap_set_float(candy_wrap_t wrap, candy_float_t value);
candy_wrap_t candy_wrap_set_boolean(candy_wrap_t wrap, candy_boolean_t value);
candy_wrap_t candy_wrap_set_method(candy_wrap_t wrap, candy_method_t method);
candy_wrap_t candy_wrap_set_string(candy_wrap_t wrap, candy_string_t string);

/* wrap get methods */

candy_integer_t candy_wrap_get_integer(candy_wrap_t wrap);
candy_float_t candy_wrap_get_float(candy_wrap_t wrap);
candy_boolean_t candy_wrap_get_boolean(candy_wrap_t wrap);
candy_method_t candy_wrap_get_method(candy_wrap_t wrap);
candy_string_t candy_wrap_get_string(candy_wrap_t wrap);

#ifdef __cplusplus
}
#endif /* __cplusplus */
#endif /* CANDY_SRC_STRUCT_WRAP_H */
