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
#include "candy_object.h"
#include "src/platform/candy_memory.h"
#include "src/platform/candy_lib.h"

struct candy_object{
  candy_queue_t member;
};

candy_object_t candy_object_create(void){
  candy_object_t obj = (candy_object_t)candy_malloc(sizeof(struct candy_object));
  obj->member = candy_queue_create();
  return obj;
}

candy_object_t candy_object_delete(candy_object_t obj){
  if (obj != NULL){
    candy_queue_delete(obj->member);
    candy_free(obj);
  }
  return NULL;
}

void candy_object_print(candy_object_t obj){
  candy_assert(obj != NULL);
  candy_queue_print(obj->member);
}

int candy_object_register(candy_object_t obj, candy_register_t reg[]){
  candy_assert(obj != NULL);
  candy_assert(reg != NULL);
  for (; reg->name != NULL && reg->method != NULL; reg++)
    candy_object_push_method(obj, reg->name, reg->method);
  return 1;
}

int candy_object_push(candy_object_t obj, candy_pack_t pack){
  candy_assert(obj != NULL);
  candy_enqueue(obj->member, pack, 0);
  return 1;
}

int candy_object_pop(candy_object_t obj, char *name){
  candy_assert(obj != NULL);
  return candy_dequeue_byname(obj->member, name);
}

candy_string_t candy_object_get_string(candy_object_t obj, char *name, uint16_t *size){
  candy_assert(obj != NULL);
  candy_pack_t *pack = candy_queue_search(obj->member, name);
  return (pack == NULL) ? NULL : candy_pack_get_string(*pack, size);
}

candy_integer_t candy_object_get_integer(candy_object_t obj, char *name){
  candy_assert(obj != NULL);
  candy_pack_t *pack = candy_queue_search(obj->member, name);
  return (pack == NULL) ? 0 : candy_pack_get_integer(*pack);
}

candy_float_t candy_object_get_float(candy_object_t obj, char *name){
  candy_assert(obj != NULL);
  candy_pack_t *pack = candy_queue_search(obj->member, name);
  return (pack == NULL) ? 0.0f : candy_pack_get_float(*pack);
}

candy_method_t candy_object_get_method(candy_object_t obj, char *name){
  candy_assert(obj != NULL);
  candy_pack_t *pack = candy_queue_search(obj->member, name);
  return (pack == NULL) ? NULL : candy_pack_get_method(*pack);
}

candy_object_t candy_object_get_object(candy_object_t obj, char *name){
  candy_assert(obj != NULL);
  candy_pack_t *pack = candy_queue_search(obj->member, name);
  return (pack == NULL) ? NULL : candy_pack_get_object(*pack);
}
