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
  uint8_t type;
  uint16_t size;
  candy_hash_t hash;
  candy_pack_t next;
  uint8_t buffer[];
};

static candy_pack_t candy_pack_create_byhash(candy_hash_t hash, uint16_t size, void *data, candy_builtin_t type){
  candy_pack_t pack = (candy_pack_t)candy_malloc(sizeof(struct candy_pack) + size);
  pack->type = type;
  pack->size = size;
  pack->hash = hash;
  pack->next = NULL;
  (data == NULL) ? candy_memset(pack->buffer, 0, size) : candy_memcpy(pack->buffer, data, size);
  return pack;
}

static candy_pack_t candy_pack_create(char *name, uint16_t size, void *data, candy_builtin_t type){
  return candy_pack_create_byhash(candy_time33(name), size, data, type);
}

candy_pack_t candy_pack_none(char *name){
  return candy_pack_create(name, 0, NULL, CANDY_BUILTIN_NONE);
}

candy_pack_t candy_pack_string(char *name, candy_string_t str, uint16_t size){
  return candy_pack_create(name, size, str, CANDY_BUILTIN_STRING);
}

candy_pack_t candy_pack_integer(char *name, candy_integer_t value){
  return candy_pack_create(name, sizeof(candy_integer_t), &value, CANDY_BUILTIN_INTEGER);
}

candy_pack_t candy_pack_float(char *name, candy_float_t value){
  return candy_pack_create(name, sizeof(candy_float_t), &value, CANDY_BUILTIN_FLOAT);
}

candy_pack_t candy_pack_method(char *name, candy_method_t method){
  return candy_pack_create(name, sizeof(candy_method_t), &method, CANDY_BUILTIN_METHOD);
}

candy_pack_t candy_pack_object(char *name, candy_object_t obj){
  return candy_pack_create(name, sizeof(candy_object_t), &obj, CANDY_BUILTIN_OBJECT);
}

candy_pack_t candy_pack_delete(candy_pack_t pack){
  candy_pack_t temp = NULL;
  if (pack != NULL){
    temp = pack->next;
    //if (pack->type == CANDY_BUILTIN_OBJECT && *(candy_object_t *)pack->buffer != NULL)
    //  *(candy_object_t *)pack->buffer = candy_object_delete(*(candy_object_t *)pack->buffer);
    candy_free(pack);
  }
  return temp;
}

int candy_pack_checkout(candy_pack_t pack, char *name){
  candy_assert(pack != NULL);
  return (pack->hash == candy_time33(name));
}

candy_pack_t candy_pack_copy(candy_pack_t pack){
  candy_assert(pack != NULL);
  return candy_pack_create_byhash(pack->hash, pack->size, pack->buffer, pack->type);
}

candy_pack_t *candy_pack_get_next(candy_pack_t pack){
  candy_assert(pack != NULL);
  return &pack->next;
}

candy_builtin_t candy_pack_get_type(candy_pack_t pack){
  candy_assert(pack != NULL);
  return (candy_builtin_t)pack->type;
}

uint16_t candy_pack_get_size(candy_pack_t pack){
  candy_assert(pack != NULL);
  return pack->size;
}

candy_pack_t candy_pack_set_none(candy_pack_t pack){
  candy_assert(pack != NULL);
  candy_hash_t hash = pack->hash;
  switch (pack->type){
    case CANDY_BUILTIN_NONE:
      return pack;
    default:
      hash = pack->hash;
      candy_free(pack);
      return candy_pack_create_byhash(hash, 0, NULL, CANDY_BUILTIN_NONE);
  }
}

candy_pack_t candy_pack_set_string(candy_pack_t pack, char *str, uint16_t size){
  candy_assert(pack != NULL);
  candy_hash_t hash = pack->hash;
  candy_free(pack);
  return candy_pack_create_byhash(hash, size, str, CANDY_BUILTIN_STRING);
}

candy_pack_t candy_pack_set_integer(candy_pack_t pack, candy_integer_t value){
  candy_assert(pack != NULL);
  candy_hash_t hash;
  switch (pack->type){
    case CANDY_BUILTIN_INTEGER:
      *((candy_integer_t *)pack->buffer) = value;
      return pack;
    default:
      hash = pack->hash;
      candy_free(pack);
      return candy_pack_create_byhash(hash, sizeof(candy_integer_t), &value, CANDY_BUILTIN_INTEGER);
  }
}

candy_pack_t candy_pack_set_float(candy_pack_t pack, candy_float_t value){
  candy_assert(pack != NULL);
  candy_hash_t hash;
  switch (pack->type){
    case CANDY_BUILTIN_FLOAT:
      *((candy_float_t *)pack->buffer) = value;
      return pack;
    default:
      hash = pack->hash;
      candy_free(pack);
      return candy_pack_create_byhash(hash, sizeof(candy_float_t), &value, CANDY_BUILTIN_FLOAT);
  }
}

candy_pack_t candy_pack_set_method(candy_pack_t pack, candy_method_t method){
  candy_assert(pack != NULL);
  candy_hash_t hash;
  switch (pack->type){
    case CANDY_BUILTIN_METHOD:
      *((candy_method_t *)pack->buffer) = method;
      return pack;
    default:
      hash = pack->hash;
      candy_free(pack);
      return candy_pack_create_byhash(hash, sizeof(candy_method_t), &method, CANDY_BUILTIN_METHOD);
  }
}

candy_pack_t candy_pack_set_object(candy_pack_t pack, candy_object_t obj){
  candy_assert(pack != NULL);
  candy_hash_t hash;
  switch (pack->type){
    case CANDY_BUILTIN_OBJECT:
      *((candy_object_t *)pack->buffer) = obj;
      return pack;
    default:
      hash = pack->hash;
      candy_free(pack);
      return candy_pack_create_byhash(hash, sizeof(candy_method_t), &obj, CANDY_BUILTIN_OBJECT);
  }
}

void *candy_pack_get_none(candy_pack_t pack){
  candy_assert(pack != NULL);
  switch (pack->type){
    case CANDY_BUILTIN_NONE:
      return (void *)pack->buffer;
    default:
      return NULL;
  }
}

candy_string_t candy_pack_get_string(candy_pack_t pack, uint16_t *size){
  candy_assert(pack != NULL);
  switch (pack->type){
    case CANDY_BUILTIN_STRING:
      if (size != NULL) *size = pack->size;
      return (candy_string_t)pack->buffer;
    default:
      return NULL;
  }
}

candy_integer_t candy_pack_get_integer(candy_pack_t pack){
#define round(x) ((x) >= 0 ? (long)((x) + 0.5) : (long)((x) - 0.5))
  candy_assert(pack != NULL);
  switch (pack->type){
    case CANDY_BUILTIN_INTEGER:
      return *(candy_integer_t *)pack->buffer;
    case CANDY_BUILTIN_FLOAT:
      return (candy_integer_t)round(*(candy_float_t *)pack->buffer);
    default:
      return 0;
  }
#undef round
}

candy_float_t candy_pack_get_float(candy_pack_t pack){
  candy_assert(pack != NULL);
  switch (pack->type){
    case CANDY_BUILTIN_FLOAT:
      return *(candy_float_t *)pack->buffer;
    case CANDY_BUILTIN_INTEGER:
      return (candy_float_t)(*(candy_integer_t *)pack->buffer);
    default:
      return 0.0f;
  }
}

candy_method_t candy_pack_get_method(candy_pack_t pack){
  candy_assert(pack != NULL);
  switch (pack->type){
    case CANDY_BUILTIN_METHOD:
      return *(candy_method_t *)pack->buffer;
    default:
      return NULL;
  }
}

candy_object_t candy_pack_get_object(candy_pack_t pack){
  candy_assert(pack != NULL);
  switch (pack->type){
    case CANDY_BUILTIN_OBJECT:
      return *(candy_object_t *)pack->buffer;
    default:
      return NULL;
  }
}
