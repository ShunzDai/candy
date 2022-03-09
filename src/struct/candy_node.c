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
#include "candy_node.h"
#include "src/platform/candy_memory.h"
#include "src/platform/candy_lib.h"

struct candy_node{
  uint8_t type;
  candy_hash_t hash;
  candy_node_t next;
  uint8_t buffer[];
};

static candy_node_t candy_node_create_byhash(candy_hash_t hash, void *data, candy_builtin_t type){
  size_t size = 0;
  switch (type){
    case CANDY_BUILTIN_NONE:
      size = 0;
      break;
    case CANDY_BUILTIN_STRING:
      size = candy_strlen((candy_string_t *)data) + 1;
      break;
    case CANDY_BUILTIN_INTEGER:
      size = sizeof(candy_integer_t);
      break;
    case CANDY_BUILTIN_FLOAT:
      size = sizeof(candy_float_t);
      break;
    case CANDY_BUILTIN_METHOD:
      size = sizeof(candy_method_t);
      break;
    default:
      break;
  }
  candy_node_t node = (candy_node_t)candy_malloc(sizeof(struct candy_node) + size);
  node->type = type;
  node->hash = hash;
  node->next = NULL;
  (data == NULL) ? candy_memset(node->buffer, 0, size) : candy_memcpy(node->buffer, data, size);
  return node;
}

static candy_node_t candy_node_create(char *name, void *data, candy_builtin_t type){
  return candy_node_create_byhash(candy_time33(name), data, type);
}

candy_node_t candy_create_none(char *name){
  return candy_node_create(name, NULL, CANDY_BUILTIN_NONE);
}

candy_node_t candy_create_string(char *name, candy_string_t *str){
  return candy_node_create(name, str, CANDY_BUILTIN_STRING);
}

candy_node_t candy_create_integer(char *name, candy_integer_t value){
  return candy_node_create(name, &value, CANDY_BUILTIN_INTEGER);
}

candy_node_t candy_create_float(char *name, candy_float_t value){
  return candy_node_create(name, &value, CANDY_BUILTIN_FLOAT);
}

candy_node_t candy_create_method(char *name, candy_method_t method){
  return candy_node_create(name, &method, CANDY_BUILTIN_METHOD);
}

candy_node_t candy_node_delete(candy_node_t node){
  candy_node_t temp = NULL;
  if (node != NULL){
    temp = node->next;
    candy_free(node);
  }
  return temp;
}

int candy_node_checkout(candy_node_t node, char *name){
  candy_assert(node != NULL);
  return (node->hash == candy_time33(name));
}

candy_node_t *candy_node_get_next(candy_node_t node){
  candy_assert(node != NULL);
  return &node->next;
}

candy_node_t candy_node_set_none(candy_node_t node){
  candy_assert(node != NULL);
  candy_hash_t hash = node->hash;
  switch (node->type){
    case CANDY_BUILTIN_NONE:
      return node;
    default:
      hash = node->hash;
      candy_free(node);
      return candy_node_create_byhash(hash, NULL, CANDY_BUILTIN_NONE);
  }
}

candy_node_t candy_node_set_string(candy_node_t node, char *str){
  candy_assert(node != NULL);
  candy_hash_t hash = node->hash;
  candy_free(node);
  return candy_node_create_byhash(hash, str, CANDY_BUILTIN_STRING);
}

candy_node_t candy_node_set_integer(candy_node_t node, candy_integer_t value){
  candy_assert(node != NULL);
  candy_hash_t hash;
  switch (node->type){
    case CANDY_BUILTIN_INTEGER:
      *((candy_integer_t *)node->buffer) = value;
      return node;
    default:
      hash = node->hash;
      candy_free(node);
      return candy_node_create_byhash(hash, &value, CANDY_BUILTIN_INTEGER);
  }
}

candy_node_t candy_node_set_float(candy_node_t node, float value){
  candy_assert(node != NULL);
  candy_hash_t hash;
  switch (node->type){
    case CANDY_BUILTIN_FLOAT:
      *((float *)node->buffer) = value;
      return node;
    default:
      hash = node->hash;
      candy_free(node);
      return candy_node_create_byhash(hash, &value, CANDY_BUILTIN_FLOAT);
  }
}

candy_node_t candy_node_set_method(candy_node_t node, candy_method_t method){
  candy_assert(node != NULL);
  candy_hash_t hash;
  switch (node->type){
    case CANDY_BUILTIN_METHOD:
      *((candy_method_t *)node->buffer) = method;
      return node;
    default:
      hash = node->hash;
      candy_free(node);
      return candy_node_create_byhash(hash, &method, CANDY_BUILTIN_METHOD);
  }
}

char *candy_node_get_string(candy_node_t node){
  candy_assert(node != NULL);
  return node->type == CANDY_BUILTIN_STRING ? (char *)node->buffer : "";
}

candy_integer_t candy_node_get_integer(candy_node_t node){
  candy_assert(node != NULL);
  return node->type == CANDY_BUILTIN_INTEGER ? *(candy_integer_t *)node->buffer : 0;
}

float candy_node_get_float(candy_node_t node){
  candy_assert(node != NULL);
  return node->type == CANDY_BUILTIN_FLOAT ? *(float *)node->buffer : 0.0f;
}

candy_method_t candy_node_get_method(candy_node_t node){
  candy_assert(node != NULL);
  return node->type == CANDY_BUILTIN_METHOD ? *(candy_method_t *)node->buffer : NULL;
}
