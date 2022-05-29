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
#include "candy_pack.h"
#include "src/common/candy_lib.h"
#include "src/platform/candy_memory.h"
#include "src/struct/candy_object.h"

#define PRIVATE_COMMON_TYPE  4
#define PRIVATE_COMMON_SZIE 12
#define PRIVATE_COMMON_ARGS 16

#if (PRIVATE_COMMON_TYPE + PRIVATE_COMMON_SZIE + PRIVATE_COMMON_ARGS) != 32
#error "private common struct size error"
#endif

#define _private(pack) ((struct private *)((pack)->private))

struct candy_pack{
  candy_node_t next;
  uint8_t private[];
};

struct private{
  uint32_t type : PRIVATE_COMMON_TYPE;
  uint32_t size : PRIVATE_COMMON_SZIE;
  uint32_t      : PRIVATE_COMMON_ARGS;
  candy_hash_t hash;
  uint8_t data[];
};

static candy_pack_t _candy_pack_create(candy_hash_t hash, candy_span_t space, candy_types_t type, candy_node_t next){
  candy_pack_t pack = (candy_pack_t)candy_malloc(sizeof(struct candy_pack) + sizeof(struct private) + space.size);
  pack->next = next;
  _private(pack)->size = space.size;
  _private(pack)->type = type;
  _private(pack)->hash = hash;
  (space.data == NULL) ? candy_memset(_private(pack)->data, 0, space.size) : candy_memcpy(_private(pack)->data, space.data, space.size);
  return pack;
}

candy_span_t candy_pack_observe(candy_pack_t pack){
  candy_assert(pack != NULL);
  return (candy_span_t){_private(pack)->data, _private(pack)->size};
}

inline candy_types_t candy_pack_type(candy_pack_t pack){
  candy_assert(pack != NULL);
  return _private(pack)->type;
}

inline bool candy_pack_match(candy_pack_t pack, candy_hash_t hash){
  candy_assert(pack != NULL);
  return (_private(pack)->hash == hash);
}

candy_pack_t candy_pack_delete(candy_pack_t pack){
  candy_pack_t temp = NULL;
  if (pack != NULL){
    temp = (candy_pack_t)pack->next;
    switch (_private(pack)->type){
      case CANDY_TYPE_OBJECT:
        candy_queue_delete(*(candy_queue_t *)_private(pack)->data);
        break;
      default:
        break;
    }
    candy_free(pack);
  }
  return temp;
}

candy_pack_t candy_pack_copy(candy_pack_t pack){
  candy_assert(pack != NULL);
  return _candy_pack_create(_private(pack)->hash, (candy_span_t){_private(pack)->data, _private(pack)->size}, _private(pack)->type, NULL);
}

inline candy_pack_t candy_pack_none(candy_hash_t hash){
  return _candy_pack_create(hash, (candy_span_t){NULL, 0}, CANDY_TYPE_NONE, NULL);
}

inline candy_pack_t candy_pack_string(candy_hash_t hash, candy_string_t string){
  return _candy_pack_create(hash, (candy_span_t){string.data, string.size}, CANDY_TYPE_STRING, NULL);
}

inline candy_pack_t candy_pack_integer(candy_hash_t hash, candy_integer_t value){
  return _candy_pack_create(hash, (candy_span_t){&value, sizeof(candy_integer_t)}, CANDY_TYPE_INTEGER, NULL);
}

inline candy_pack_t candy_pack_float(candy_hash_t hash, candy_float_t value){
  return _candy_pack_create(hash, (candy_span_t){&value, sizeof(candy_float_t)}, CANDY_TYPE_FLOAT, NULL);
}

inline candy_pack_t candy_pack_boolean(candy_hash_t hash, candy_boolean_t value){
  return _candy_pack_create(hash, (candy_span_t){&value, sizeof(candy_boolean_t)}, CANDY_TYPE_BOOLEAN, NULL);
}

inline candy_pack_t candy_pack_method(candy_hash_t hash, candy_method_t method){
  return _candy_pack_create(hash, (candy_span_t){&method, sizeof(candy_method_t)}, CANDY_TYPE_METHOD, NULL);
}

inline candy_pack_t candy_pack_object(candy_hash_t hash, candy_object_t object){
  return _candy_pack_create(hash, (candy_span_t){&object, sizeof(candy_object_t)}, CANDY_TYPE_OBJECT, NULL);
}

candy_pack_t candy_pack_set_none(candy_pack_t pack){
  candy_assert(pack != NULL);
  switch (_private(pack)->type){
    case CANDY_TYPE_NONE:
      return pack;
    default: {
      candy_node_t next = pack->next;
      candy_hash_t hash = _private(pack)->hash;
      candy_free(pack);
      return _candy_pack_create(hash, (candy_span_t){NULL, 0}, CANDY_TYPE_NONE, next);
    }
  }
}

candy_pack_t candy_pack_set_string(candy_pack_t pack, candy_string_t string){
  candy_assert(pack != NULL);
  candy_node_t next = pack->next;
  candy_hash_t hash = _private(pack)->hash;
  candy_free(pack);
  return _candy_pack_create(hash, (candy_span_t){string.data, string.size}, CANDY_TYPE_STRING, next);
}

candy_pack_t candy_pack_set_integer(candy_pack_t pack, candy_integer_t value){
  candy_assert(pack != NULL);
  switch (_private(pack)->type){
    case CANDY_TYPE_INTEGER:
      *((candy_integer_t *)_private(pack)->data) = value;
      return pack;
    default: {
      candy_node_t next = pack->next;
      candy_hash_t hash = _private(pack)->hash;
      candy_free(pack);
      return _candy_pack_create(hash, (candy_span_t){&value, sizeof(candy_integer_t)}, CANDY_TYPE_INTEGER, next);
    }
  }
}

candy_pack_t candy_pack_set_float(candy_pack_t pack, candy_float_t value){
  candy_assert(pack != NULL);
  switch (_private(pack)->type){
    case CANDY_TYPE_FLOAT:
      *((candy_float_t *)_private(pack)->data) = value;
      return pack;
    default:{
      candy_node_t next = pack->next;
      candy_hash_t hash = _private(pack)->hash;
      candy_free(pack);
      return _candy_pack_create(hash, (candy_span_t){&value, sizeof(candy_float_t)}, CANDY_TYPE_FLOAT, next);
    }
  }
}

candy_pack_t candy_pack_set_boolean(candy_pack_t pack, candy_boolean_t value){/* if value % 256 == 0 ? */
  candy_assert(pack != NULL);
  switch (_private(pack)->type){
    case CANDY_TYPE_BOOLEAN:
      *((candy_boolean_t *)_private(pack)->data) = value;
      return pack;
    default: {
      candy_node_t next = pack->next;
      candy_hash_t hash = _private(pack)->hash;
      candy_free(pack);
      return _candy_pack_create(hash, (candy_span_t){&value, sizeof(candy_boolean_t)}, CANDY_TYPE_BOOLEAN, next);
    }
  }
}

candy_pack_t candy_pack_set_method(candy_pack_t pack, candy_method_t method){
  candy_assert(pack != NULL);
  switch (_private(pack)->type){
    case CANDY_TYPE_METHOD:
      *((candy_method_t *)_private(pack)->data) = method;
      return pack;
    default: {
      candy_node_t next = pack->next;
      candy_hash_t hash = _private(pack)->hash;
      candy_free(pack);
      return _candy_pack_create(hash, (candy_span_t){&method, sizeof(candy_method_t)}, CANDY_TYPE_METHOD, next);
    }
  }
}

candy_pack_t candy_pack_set_object(candy_pack_t pack, candy_object_t object){
  candy_assert(pack != NULL);
  switch (_private(pack)->type){
    case CANDY_TYPE_OBJECT:
      *((candy_object_t *)_private(pack)->data) = object;
      return pack;
    default: {
      candy_node_t next = pack->next;
      candy_hash_t hash = _private(pack)->hash;
      candy_free(pack);
      return _candy_pack_create(hash, (candy_span_t){&object, sizeof(candy_object_t)}, CANDY_TYPE_OBJECT, next);
    }
  }
}

candy_string_t candy_pack_get_string(candy_pack_t pack){
  candy_assert(pack != NULL);
  switch (_private(pack)->type){
    case CANDY_TYPE_STRING:
      return (candy_string_t){_private(pack)->data, _private(pack)->size};
    default:
      candy_assert(false);
      return (candy_string_t){NULL, 0};
  }
}

candy_integer_t candy_pack_get_integer(candy_pack_t pack){
#define _round(x) ((x) >= 0 ? (candy_integer_t)((x) + 0.5) : (candy_integer_t)((x) - 0.5))
  candy_assert(pack != NULL);
  switch (_private(pack)->type){
    case CANDY_TYPE_INTEGER:
      return *(candy_integer_t *)_private(pack)->data;
    case CANDY_TYPE_FLOAT:
      return _round(*(candy_float_t *)_private(pack)->data);
    case CANDY_TYPE_BOOLEAN:
      return *(candy_boolean_t *)_private(pack)->data ? 1 : 0;
    default:
      candy_assert(false);
      return -1;
  }
#undef _round
}

candy_float_t candy_pack_get_float(candy_pack_t pack){
  candy_assert(pack != NULL);
  switch (_private(pack)->type){
    case CANDY_TYPE_FLOAT:
      return *(candy_float_t *)_private(pack)->data;
    case CANDY_TYPE_INTEGER:
      return (candy_float_t)(*(candy_integer_t *)_private(pack)->data);
    case CANDY_TYPE_BOOLEAN:
      return *(candy_boolean_t *)_private(pack)->data ? 1.0f : 0.0f;
    default:
      candy_assert(false);
      return -1;
  }
}

candy_boolean_t candy_pack_get_boolean(candy_pack_t pack){
  candy_assert(pack != NULL);
  switch (_private(pack)->type){
    case CANDY_TYPE_BOOLEAN:
      return *(candy_boolean_t *)_private(pack)->data;
    case CANDY_TYPE_INTEGER:
      return (candy_boolean_t)(*(candy_integer_t *)_private(pack)->data);
    default:
      candy_assert(false);
      return -1;
  }
}

candy_method_t candy_pack_get_method(candy_pack_t pack){
  candy_assert(pack != NULL);
  switch (_private(pack)->type){
    case CANDY_TYPE_METHOD:
      return *(candy_method_t *)_private(pack)->data;
    default:
      candy_assert(false);
      return NULL;
  }
}

candy_object_t candy_pack_get_object(candy_pack_t pack){
  candy_assert(pack != NULL);
  switch (_private(pack)->type){
    case CANDY_TYPE_OBJECT:
      return *(candy_object_t *)_private(pack)->data;
    default:
      candy_assert(false);
      return NULL;
  }
}
