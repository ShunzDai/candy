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
#include "src/platform/candy_memory.h"
#include "src/platform/candy_lib.h"
#include "src/struct/candy_object.h"

struct candy_pack{
  candy_node_t next;
  candy_hash_t hash;
  struct{
    uint32_t size : 16;
    uint32_t type : 4;
    uint32_t      : 12;
  };
  uint8_t buffer[];
};

static candy_pack_t candy_pack_create_byhash(candy_hash_t hash, const void *data, uint16_t size, candy_types_t type, candy_pack_t next){
  candy_pack_t pack = (candy_pack_t)candy_malloc(sizeof(struct candy_pack) + size);
  pack->next = (candy_node_t)next;
  pack->hash = hash;
  pack->type = type;
  pack->size = size;
  (data == NULL) ? candy_memset(pack->buffer, 0, size) : candy_memcpy(pack->buffer, data, size);
  return pack;
}

static candy_pack_t candy_pack_create(char *name, const void *data, uint16_t size, candy_types_t type, candy_pack_t next){
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

candy_pack_t candy_pack_delete(candy_pack_t pack){
  candy_pack_t temp = NULL;
  if (pack != NULL){
    temp = (candy_pack_t)pack->next;
    candy_free(pack);
  }
  return temp;
}

int candy_pack_checkout(candy_pack_t pack, char *name){
  candy_assert(pack != NULL, "");
  return (pack->hash == candy_time33(name));
}

candy_pack_t candy_pack_copy(candy_pack_t pack){
  candy_assert(pack != NULL, "");
  return candy_pack_create_byhash(pack->hash, pack->buffer, pack->size, pack->type, NULL);
}

candy_types_t candy_pack_get_type(candy_pack_t pack){
  candy_assert(pack != NULL, "");
  return (candy_types_t)pack->type;
}

const char *candy_pack_get_typestr(candy_pack_t pack){
  return candy_types[pack->type < CANDY_TYPES_FLOOR ? pack->type : CANDY_TYPES_FLOOR];
}

uint16_t candy_pack_get_size(candy_pack_t pack){
  candy_assert(pack != NULL, "");
  return pack->size;
}

candy_pack_t candy_pack_set_none(candy_pack_t pack){
  candy_assert(pack != NULL, "");
  candy_hash_t hash;
  candy_pack_t next;
  switch (pack->type){
    case CANDY_TYPES_NONE:
      return pack;
    default:
      hash = pack->hash;
      next = (candy_pack_t)pack->next;
      candy_free(pack);
      return candy_pack_create_byhash(hash, NULL, 0, CANDY_TYPES_NONE, next);
  }
}

candy_pack_t candy_pack_set_string(candy_pack_t pack, char *string, uint16_t size){
  candy_assert(pack != NULL, "");
  candy_hash_t hash = pack->hash;
  candy_pack_t next = (candy_pack_t)pack->next;
  candy_free(pack);
  return candy_pack_create_byhash(hash, string, size, CANDY_TYPES_STRING, next);
}

candy_pack_t candy_pack_set_integer(candy_pack_t pack, candy_integer_t value){
  candy_assert(pack != NULL, "");
  candy_hash_t hash;
  candy_pack_t next;
  switch (pack->type){
    case CANDY_TYPES_INTEGER:
      *((candy_integer_t *)pack->buffer) = value;
      return pack;
    default:
      hash = pack->hash;
      next = (candy_pack_t)pack->next;
      candy_free(pack);
      return candy_pack_create_byhash(hash, &value, sizeof(candy_integer_t), CANDY_TYPES_INTEGER, next);
  }
}

candy_pack_t candy_pack_set_float(candy_pack_t pack, candy_float_t value){
  candy_assert(pack != NULL, "");
  candy_hash_t hash;
  candy_pack_t next;
  switch (pack->type){
    case CANDY_TYPES_FLOAT:
      *((candy_float_t *)pack->buffer) = value;
      return pack;
    default:
      hash = pack->hash;
      next = (candy_pack_t)pack->next;
      candy_free(pack);
      return candy_pack_create_byhash(hash, &value, sizeof(candy_float_t), CANDY_TYPES_FLOAT, next);
  }
}

candy_pack_t candy_pack_set_boolean(candy_pack_t pack, candy_boolean_t value){/* if value % 256 == 0 ? */
  candy_assert(pack != NULL, "");
  candy_hash_t hash;
  candy_pack_t next;
  switch (pack->type){
    case CANDY_TYPES_BOOLEAN:
      *((candy_boolean_t *)pack->buffer) = value;
      return pack;
    default:
      hash = pack->hash;
      next = (candy_pack_t)pack->next;
      candy_free(pack);
      return candy_pack_create_byhash(hash, &value, sizeof(candy_boolean_t), CANDY_TYPES_BOOLEAN, next);
  }
}

candy_pack_t candy_pack_set_method(candy_pack_t pack, candy_method_t method){
  candy_assert(pack != NULL, "");
  candy_hash_t hash;
  candy_pack_t next;
  switch (pack->type){
    case CANDY_TYPES_METHOD:
      *((candy_method_t *)pack->buffer) = method;
      return pack;
    default:
      hash = pack->hash;
      next = (candy_pack_t)pack->next;
      candy_free(pack);
      return candy_pack_create_byhash(hash, &method, sizeof(candy_method_t), CANDY_TYPES_METHOD, next);
  }
}

candy_pack_t candy_pack_set_object(candy_pack_t pack, candy_object_t object){
  candy_assert(pack != NULL, "");
  candy_hash_t hash;
  candy_pack_t next;
  switch (pack->type){
    case CANDY_TYPES_OBJECT:
      *((candy_object_t *)pack->buffer) = object;
      return pack;
    default:
      hash = pack->hash;
      next = (candy_pack_t)pack->next;
      candy_free(pack);
      return candy_pack_create_byhash(hash, &object, sizeof(candy_method_t), CANDY_TYPES_OBJECT, next);
  }
}

candy_string_t candy_pack_get_string(candy_pack_t pack, uint16_t *size){
  candy_assert(pack != NULL, "");
  switch (pack->type){
    case CANDY_TYPES_STRING:
      if (size != NULL) *size = pack->size;
      return (candy_string_t)pack->buffer;
    default:
      candy_assert(0, "'%s' cannot be converted to '%s'", candy_pack_get_typestr(pack), candy_types[CANDY_TYPES_STRING]);
      return NULL;
  }
}

candy_integer_t candy_pack_get_integer(candy_pack_t pack){
#define round(x) ((x) >= 0 ? (long)((x) + 0.5) : (long)((x) - 0.5))
  candy_assert(pack != NULL, "");
  switch (pack->type){
    case CANDY_TYPES_INTEGER:
      return *(candy_integer_t *)pack->buffer;
    case CANDY_TYPES_FLOAT:
      return (candy_integer_t)round(*(candy_float_t *)pack->buffer);
    default:
      candy_assert(0, "'%s' cannot be converted to '%s'", candy_pack_get_typestr(pack), candy_types[CANDY_TYPES_INTEGER]);
      return 0;
  }
#undef round
}

candy_float_t candy_pack_get_float(candy_pack_t pack){
  candy_assert(pack != NULL, "");
  switch (pack->type){
    case CANDY_TYPES_FLOAT:
      return *(candy_float_t *)pack->buffer;
    case CANDY_TYPES_INTEGER:
      return (candy_float_t)(*(candy_integer_t *)pack->buffer);
    default:
      candy_assert(0, "'%s' cannot be converted to '%s'", candy_pack_get_typestr(pack), candy_types[CANDY_TYPES_FLOAT]);
      return 0;
  }
}

candy_boolean_t candy_pack_get_boolean(candy_pack_t pack){
  candy_assert(pack != NULL, "");
  switch (pack->type){
    case CANDY_TYPES_BOOLEAN:
      return *(candy_boolean_t *)pack->buffer;
    case CANDY_TYPES_INTEGER:
      return (candy_boolean_t)(*(candy_integer_t *)pack->buffer);
    default:
      candy_assert(0, "'%s' cannot be converted to '%s'", candy_pack_get_typestr(pack), candy_types[CANDY_TYPES_BOOLEAN]);
      return 0;
  }
}

candy_method_t candy_pack_get_method(candy_pack_t pack){
  candy_assert(pack != NULL, "");
  switch (pack->type){
    case CANDY_TYPES_METHOD:
      return *(candy_method_t *)pack->buffer;
    default:
      candy_assert(0, "'%s' cannot be converted to '%s'", candy_pack_get_typestr(pack), candy_types[CANDY_TYPES_METHOD]);
      return NULL;
  }
}

candy_object_t candy_pack_get_object(candy_pack_t pack){
  candy_assert(pack != NULL, "");
  switch (pack->type){
    case CANDY_TYPES_OBJECT:
      return *(candy_object_t *)pack->buffer;
    default:
      candy_assert(0, "'%s' cannot be converted to '%s'", candy_pack_get_typestr(pack), candy_types[CANDY_TYPES_OBJECT]);
      return NULL;
  }
}
