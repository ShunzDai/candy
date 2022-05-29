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
#ifndef CANDY_SRC_STRUCT_PACK_H
#define CANDY_SRC_STRUCT_PACK_H
#ifdef __cplusplus
extern "C"{
#endif /* __cplusplus */

#include "src/common/candy_types.h"

/* pack general methods */

candy_span_t candy_pack_observe(candy_pack_t pack);
candy_types_t candy_pack_type(candy_pack_t pack);
bool candy_pack_match(candy_pack_t pack, candy_hash_t hash);

/* pack delete method */

candy_pack_t candy_pack_delete(candy_pack_t pack);

/* pack create methods */

candy_pack_t candy_pack_copy(candy_pack_t pack);
candy_pack_t candy_pack_none(candy_hash_t hash);
candy_pack_t candy_pack_string(candy_hash_t hash, candy_string_t string);
candy_pack_t candy_pack_integer(candy_hash_t hash, candy_integer_t value);
candy_pack_t candy_pack_float(candy_hash_t hash, candy_float_t value);
candy_pack_t candy_pack_boolean(candy_hash_t hash, candy_boolean_t value);
candy_pack_t candy_pack_method(candy_hash_t hash, candy_method_t method);
candy_pack_t candy_pack_object(candy_hash_t hash, candy_object_t object);

/* pack set methods */

candy_pack_t candy_pack_set_none(candy_pack_t pack);
candy_pack_t candy_pack_set_string(candy_pack_t pack, candy_string_t string);
candy_pack_t candy_pack_set_integer(candy_pack_t pack, candy_integer_t value);
candy_pack_t candy_pack_set_float(candy_pack_t pack, candy_float_t value);
candy_pack_t candy_pack_set_boolean(candy_pack_t pack, candy_boolean_t value);
candy_pack_t candy_pack_set_method(candy_pack_t pack, candy_method_t method);
candy_pack_t candy_pack_set_object(candy_pack_t pack, candy_object_t object);

/* pack get methods */

candy_string_t candy_pack_get_string(candy_pack_t pack);
candy_integer_t candy_pack_get_integer(candy_pack_t pack);
candy_float_t candy_pack_get_float(candy_pack_t pack);
candy_boolean_t candy_pack_get_boolean(candy_pack_t pack);
candy_method_t candy_pack_get_method(candy_pack_t pack);
candy_object_t candy_pack_get_object(candy_pack_t pack);

#ifdef __cplusplus
}
#endif /* __cplusplus */
#endif /* CANDY_SRC_STRUCT_PACK_H */
