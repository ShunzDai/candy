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
  struct candy_node *next;
  uint8_t buffer[];
};

static candy_hash_t candy_gen_hash(char *str){
  candy_hash_t hash = 5381;
  while (*str){
    hash += (hash << 5) + (*str++);
  }
  return (hash & 0x7FFFFFFF);
}

static candy_node_t candy_node_create_byname(char *name, void *data, uint32_t size, candy_builtin_t type){
  candy_node_t node = (candy_node_t)candy_malloc(sizeof(struct candy_node) + size);

  node->type = type;
  node->hash = candy_gen_hash(name);
  node->next = NULL;
  (data == NULL) ? candy_memset(node->buffer, 0, size) : candy_memcpy(node->buffer, data, size);

  return node;
}

static candy_node_t candy_node_create_byhash(candy_hash_t hash, void *data, uint32_t size, candy_builtin_t type){
  candy_node_t node = (candy_node_t)candy_malloc(sizeof(struct candy_node) + size);

  node->type = type;
  node->hash = hash;
  node->next = NULL;
  (data == NULL) ? candy_memset(node->buffer, 0, size) : candy_memcpy(node->buffer, data, size);

  return node;
}

candy_node_t candy_node_create(char *name){
  return candy_node_create_byname(name, NULL, 0, CANDY_BUILTIN_NONE);
}

candy_node_t candy_node_delete(candy_node_t node){
  if (node != NULL)
    candy_free(node);
  return NULL;
}

int candy_node_checkout(candy_node_t node, char *name){
  candy_assert(node != NULL);
  return (node->hash == candy_gen_hash(name));
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
      return candy_node_create_byhash(hash, NULL, 0, CANDY_BUILTIN_NONE);
  }
}

char *candy_node_get_string(candy_node_t node){
  candy_assert(node != NULL);
  return node->type == CANDY_BUILTIN_STRING ? (char *)node->buffer : "";
}

candy_node_t candy_node_set_string(candy_node_t node, char *str){
  candy_assert(node != NULL);
  candy_hash_t hash = node->hash;
  candy_free(node);
  return candy_node_create_byhash(hash, str, candy_strlen(str) + 1, CANDY_BUILTIN_STRING);
}

int candy_node_get_integer(candy_node_t node){
  candy_assert(node != NULL);
  return node->type == CANDY_BUILTIN_INTEGER ? *(int *)node->buffer : 0;
}

candy_node_t candy_node_set_integer(candy_node_t node, int value){
  candy_assert(node != NULL);
  candy_hash_t hash;
  switch (node->type){
    case CANDY_BUILTIN_INTEGER:
      *((int *)node->buffer) = value;
      return node;
    default:
      hash = node->hash;
      candy_free(node);
      return candy_node_create_byhash(hash, &value, sizeof(value), CANDY_BUILTIN_INTEGER);
  }
}

float candy_node_get_float(candy_node_t node){
  candy_assert(node != NULL);
  return node->type == CANDY_BUILTIN_FLOAT ? *(float *)node->buffer : 0.0f;
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
      return candy_node_create_byhash(hash, &value, sizeof(value), CANDY_BUILTIN_FLOAT);
  }
}
