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

#define _private(pack) ((struct pack_private *)((pack)->priv))

struct candy_pack{
  candy_node_t next;
  uint8_t priv[];
};

struct pack_private{
  candy_node_common_bits(32);
  candy_hash_t hash;
  uint8_t buffer[];
};

static candy_pack_t candy_pack_create_byhash(candy_hash_t hash, const void *data, uint16_t size, candy_types_t type, candy_node_t next){
  candy_pack_t pack = (candy_pack_t)candy_malloc(sizeof(struct candy_pack) + sizeof(struct pack_private) + size);
  pack->next = next;
  _private(pack)->size = size;
  _private(pack)->type = type;
  _private(pack)->hash = hash;
  (data == NULL) ? candy_memset(_private(pack)->buffer, 0, size) : candy_memcpy(_private(pack)->buffer, data, size);
  return pack;
}

static inline candy_pack_t candy_pack_create(char *name, const void *data, uint16_t size, candy_types_t type, candy_node_t next){
  return candy_pack_create_byhash(candy_time33(name), data, size, type, next);
}

candy_pack_t candy_pack_none(char *name){
  return candy_pack_create(name, NULL, 0, CANDY_TYPES_NONE, NULL);
}

candy_pack_t candy_pack_string(char *name, candy_string_t string, uint16_t size){
  return candy_pack_create(name, string, size, CANDY_TYPES_STRING, NULL);
}

candy_pack_t candy_pack_integer(char *name, candy_integer_t value){
  return candy_pack_create(name, &value, sizeof(candy_integer_t), CANDY_TYPES_INTEGER, NULL);
}

candy_pack_t candy_pack_float(char *name, candy_float_t value){
  return candy_pack_create(name, &value, sizeof(candy_float_t), CANDY_TYPES_FLOAT, NULL);
}

candy_pack_t candy_pack_boolean(char *name, candy_boolean_t value){
  return candy_pack_create(name, &value, sizeof(candy_boolean_t), CANDY_TYPES_BOOLEAN, NULL);
}

candy_pack_t candy_pack_method(char *name, candy_method_t method){
  return candy_pack_create(name, &method, sizeof(candy_method_t), CANDY_TYPES_METHOD, NULL);
}

candy_pack_t candy_pack_object(char *name, candy_object_t object){
  return candy_pack_create(name, &object, sizeof(candy_object_t), CANDY_TYPES_OBJECT, NULL);
}

int candy_pack_checkout(candy_pack_t pack, candy_hash_t hash){
  candy_assert(pack != NULL);
  return (_private(pack)->hash == hash);
}

candy_pack_t candy_pack_copy(candy_pack_t pack){
  candy_assert(pack != NULL);
  return candy_pack_create_byhash(_private(pack)->hash, _private(pack)->buffer, _private(pack)->size, _private(pack)->type, NULL);
}

uint8_t *candy_pack_get_buffer(candy_pack_t pack){
  candy_assert(pack != NULL);
  return _private(pack)->buffer;
}

candy_pack_t candy_pack_set_none(candy_pack_t pack){
  candy_assert(pack != NULL);
  candy_node_t next = pack->next;
  candy_hash_t hash = _private(pack)->hash;
  switch (_private(pack)->type){
    case CANDY_TYPES_NONE:
      return pack;
    default:
      candy_free(pack);
      return candy_pack_create_byhash(hash, NULL, 0, CANDY_TYPES_NONE, next);
  }
}

candy_pack_t candy_pack_set_string(candy_pack_t pack, char *string, uint16_t size){
  candy_assert(pack != NULL);
  candy_node_t next = pack->next;
  candy_hash_t hash = _private(pack)->hash;
  candy_free(pack);
  return candy_pack_create_byhash(hash, string, size, CANDY_TYPES_STRING, next);
}

candy_pack_t candy_pack_set_integer(candy_pack_t pack, candy_integer_t value){
  candy_assert(pack != NULL);
  candy_node_t next = pack->next;
  candy_hash_t hash = _private(pack)->hash;
  switch (_private(pack)->type){
    case CANDY_TYPES_INTEGER:
      *((candy_integer_t *)_private(pack)->buffer) = value;
      return pack;
    default:
      candy_free(pack);
      return candy_pack_create_byhash(hash, &value, sizeof(candy_integer_t), CANDY_TYPES_INTEGER, next);
  }
}

candy_pack_t candy_pack_set_float(candy_pack_t pack, candy_float_t value){
  candy_assert(pack != NULL);
  candy_node_t next;
  candy_hash_t hash;
  switch (_private(pack)->type){
    case CANDY_TYPES_FLOAT:
      *((candy_float_t *)_private(pack)->buffer) = value;
      return pack;
    default:
      next = pack->next;
      hash = _private(pack)->hash;
      candy_free(pack);
      return candy_pack_create_byhash(hash, &value, sizeof(candy_float_t), CANDY_TYPES_FLOAT, next);
  }
}

candy_pack_t candy_pack_set_boolean(candy_pack_t pack, candy_boolean_t value){/* if value % 256 == 0 ? */
  candy_assert(pack != NULL);
  candy_node_t next;
  candy_hash_t hash;
  switch (_private(pack)->type){
    case CANDY_TYPES_BOOLEAN:
      *((candy_boolean_t *)_private(pack)->buffer) = value;
      return pack;
    default:
      next = pack->next;
      hash = _private(pack)->hash;
      candy_free(pack);
      return candy_pack_create_byhash(hash, &value, sizeof(candy_boolean_t), CANDY_TYPES_BOOLEAN, next);
  }
}

candy_pack_t candy_pack_set_method(candy_pack_t pack, candy_method_t method){
  candy_assert(pack != NULL);
  candy_node_t next;
  candy_hash_t hash;
  switch (_private(pack)->type){
    case CANDY_TYPES_METHOD:
      *((candy_method_t *)_private(pack)->buffer) = method;
      return pack;
    default:
      next = pack->next;
      hash = _private(pack)->hash;
      candy_free(pack);
      return candy_pack_create_byhash(hash, &method, sizeof(candy_method_t), CANDY_TYPES_METHOD, next);
  }
}

candy_pack_t candy_pack_set_object(candy_pack_t pack, candy_object_t object){
  candy_assert(pack != NULL);
  candy_node_t next;
  candy_hash_t hash;
  switch (_private(pack)->type){
    case CANDY_TYPES_OBJECT:
      *((candy_object_t *)_private(pack)->buffer) = object;
      return pack;
    default:
      next = pack->next;
      hash = _private(pack)->hash;
      candy_free(pack);
      return candy_pack_create_byhash(hash, &object, sizeof(candy_object_t), CANDY_TYPES_OBJECT, next);
  }
}

candy_string_t candy_pack_get_string(candy_pack_t pack, uint16_t *size){
  candy_assert(pack != NULL);
  switch (_private(pack)->type){
    case CANDY_TYPES_STRING:
      if (size != NULL) *size = _private(pack)->size;
      return (candy_string_t)_private(pack)->buffer;
    default:
      candy_assert(0);
      return NULL;
  }
}

candy_integer_t candy_pack_get_integer(candy_pack_t pack){
#define _round(x) ((x) >= 0 ? (candy_integer_t)((x) + 0.5) : (candy_integer_t)((x) - 0.5))
  candy_assert(pack != NULL);
  switch (_private(pack)->type){
    case CANDY_TYPES_INTEGER:
      return *(candy_integer_t *)_private(pack)->buffer;
    case CANDY_TYPES_FLOAT:
      return _round(*(candy_float_t *)_private(pack)->buffer);
    case CANDY_TYPES_BOOLEAN:
      return *(candy_boolean_t *)_private(pack)->buffer ? 1 : 0;
    default:
      candy_assert(0);
      return 0;
  }
#undef _round
}

candy_float_t candy_pack_get_float(candy_pack_t pack){
  candy_assert(pack != NULL);
  switch (_private(pack)->type){
    case CANDY_TYPES_FLOAT:
      return *(candy_float_t *)_private(pack)->buffer;
    case CANDY_TYPES_INTEGER:
      return (candy_float_t)(*(candy_integer_t *)_private(pack)->buffer);
    case CANDY_TYPES_BOOLEAN:
      return *(candy_boolean_t *)_private(pack)->buffer ? 1.0f : 0.0f;
    default:
      candy_assert(0);
      return 0;
  }
}

candy_boolean_t candy_pack_get_boolean(candy_pack_t pack){
  candy_assert(pack != NULL);
  switch (_private(pack)->type){
    case CANDY_TYPES_BOOLEAN:
      return *(candy_boolean_t *)_private(pack)->buffer;
    case CANDY_TYPES_INTEGER:
      return (candy_boolean_t)(*(candy_integer_t *)_private(pack)->buffer);
    default:
      candy_assert(0);
      return 0;
  }
}

candy_method_t candy_pack_get_method(candy_pack_t pack){
  candy_assert(pack != NULL);
  switch (_private(pack)->type){
    case CANDY_TYPES_METHOD:
      return *(candy_method_t *)_private(pack)->buffer;
    default:
      candy_assert(0);
      return NULL;
  }
}

candy_object_t candy_pack_get_object(candy_pack_t pack){
  candy_assert(pack != NULL);
  switch (_private(pack)->type){
    case CANDY_TYPES_OBJECT:
      return *(candy_object_t *)_private(pack)->buffer;
    default:
      candy_assert(0);
      return NULL;
  }
}
