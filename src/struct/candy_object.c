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

void candy_object_print(candy_object_t obj){
  candy_assert(obj != NULL, "");
  candy_pack_t *temp = (candy_pack_t *)candy_get_next(obj);
  uint32_t count = 0;
  uint16_t size = 0;
  printf("\033[1;35m==================== head ====================\033[0m\n");
  while (temp != NULL){
    if (*temp == NULL)
      break;
    printf("[%d]\t%p\t%s\t", count, *temp, candy_pack_get_typestr(*temp));
    switch (candy_pack_get_type(*temp)){
      case CANDY_TYPES_NONE:
        printf("\n");
        break;
      case CANDY_TYPES_STRING:
        printf("%s\n", candy_pack_get_string(*temp, &size));
        break;
      case CANDY_TYPES_INTEGER:
        printf("%ld\n", candy_pack_get_integer(*temp));
        break;
      case CANDY_TYPES_FLOAT:
        printf("%.5f\n", candy_pack_get_float(*temp));
        break;
      case CANDY_TYPES_BOOLEAN:
        printf("%s\n", candy_pack_get_boolean(*temp) ? "True" : "False");
        break;
      case CANDY_TYPES_METHOD:
        printf("%p\n", candy_pack_get_method(*temp));
        break;
      case CANDY_TYPES_OBJECT:
        printf("%p\n", candy_pack_get_object(*temp));
        break;
      default:
        printf("exception types\n");
        break;
    }
    count++;
    temp = (candy_pack_t *)candy_get_next(*temp);
  }
  printf("pack count(s): %d\n", count);
  printf("\033[1;35m==================== tail ====================\033[0m\n");
}

int candy_object_register(candy_object_t obj, candy_register_t reg[]){
  candy_assert(obj != NULL, "");
  candy_assert(reg != NULL, "");
  for (; reg->name != NULL && reg->method != NULL; reg++)
    candy_object_push_method(obj, reg->name, reg->method);
  return 1;
}

candy_pack_t *candy_object_search(candy_object_t obj, char *name){
  candy_assert(obj != NULL, "");
  candy_pack_t *temp = (candy_pack_t *)candy_get_next(obj);
  while (temp != NULL){
    if (*temp == NULL)
      return NULL;
    else if (candy_pack_checkout(*temp, name))
      break;
    temp = (candy_pack_t *)candy_get_next(*temp);
  }
  return temp;
}

int candy_object_push(candy_object_t obj, candy_pack_t pack){
  candy_assert(obj != NULL, "");
  candy_enqueue(obj, -1, (candy_node_t)pack);
  return 1;
}

candy_string_t candy_object_get_string(candy_object_t obj, char *name, uint16_t *size){
  candy_assert(obj != NULL, "");
  candy_pack_t *pack = candy_object_search(obj, name);
  return (pack == NULL) ? NULL : candy_pack_get_string(*pack, size);
}

candy_integer_t candy_object_get_integer(candy_object_t obj, char *name){
  candy_assert(obj != NULL, "");
  candy_pack_t *pack = candy_object_search(obj, name);
  return (pack == NULL) ? 0 : candy_pack_get_integer(*pack);
}

candy_float_t candy_object_get_float(candy_object_t obj, char *name){
  candy_assert(obj != NULL, "");
  candy_pack_t *pack = candy_object_search(obj, name);
  return (pack == NULL) ? 0.0f : candy_pack_get_float(*pack);
}

candy_method_t candy_object_get_method(candy_object_t obj, char *name){
  candy_assert(obj != NULL, "");
  candy_pack_t *pack = candy_object_search(obj, name);
  return (pack == NULL) ? NULL : candy_pack_get_method(*pack);
}

candy_object_t candy_object_get_object(candy_object_t obj, char *name){
  candy_assert(obj != NULL, "");
  candy_pack_t *pack = candy_object_search(obj, name);
  return (pack == NULL) ? NULL : candy_pack_get_object(*pack);
}
