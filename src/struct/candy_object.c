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

int candy_object_push(candy_object_t obj, candy_node_t node){
  candy_assert(obj != NULL);
  candy_enqueue(obj->member, node);
  return 1;
}

int candy_object_pop(candy_object_t obj, char *name){
  candy_assert(obj != NULL);
  candy_dequeue_byname(obj->member, name);
  return 1;
}

candy_method_t candy_object_get_method(candy_object_t obj, char *name){
  candy_assert(obj != NULL);
  return candy_node_get_method(*candy_queue_search(obj->member, name));
}
