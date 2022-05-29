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
#include "src/common/candy_lib.h"

static void candy_object_print_depth(candy_object_t obj, uint32_t depth){
  candy_pack_t *temp = (candy_pack_t *)candy_get_next(obj);
  uint32_t count = 0;
  while (temp != NULL){
    if (*temp == NULL)
      break;
    printf("[%d-%d]\t\t%p\t%s\t", depth, count, *temp, candy_types[candy_pack_type(*temp) < CANDY_TYPE_FLOOR ? candy_pack_type(*temp) : CANDY_TYPE_FLOOR]);
    switch (candy_pack_type(*temp)){
      case CANDY_TYPE_NONE:
        printf("\n");
        break;
      case CANDY_TYPE_STRING:
        printf("%s\n", (char *)candy_pack_get_string(*temp).data);
        break;
      case CANDY_TYPE_INTEGER:
        printf("%ld\n", candy_pack_get_integer(*temp));
        break;
      case CANDY_TYPE_FLOAT:
        printf("%.5f\n", candy_pack_get_float(*temp));
        break;
      case CANDY_TYPE_BOOLEAN:
        printf("%s\n", candy_pack_get_boolean(*temp) ? "True" : "False");
        break;
      case CANDY_TYPE_METHOD:
        printf("%p\n", candy_pack_get_method(*temp));
        break;
      case CANDY_TYPE_OBJECT:
        printf("%p\n", candy_pack_get_object(*temp));
        printf("\033[1;35m>>> sub object head\033[0m\n");
        candy_object_print_depth(candy_pack_get_object(*temp), depth + 1);
        printf("\033[1;35m<<< sub object tail\033[0m\n");
        break;
      default:
        printf("exception types\n");
        break;
    }
    count++;
    temp = (candy_pack_t *)candy_get_next(*temp);
  }
  printf("depth: %d, count(s): %d\n", depth, count);
}

void candy_object_print(candy_object_t obj){
  candy_assert(obj != NULL);
  printf("depth-pos\taddress\t\ttype\tvalue\n");
  printf("\033[1;35m>>> object head\033[0m\n");
  candy_object_print_depth(obj, 0);
  printf("\033[1;35m<<< object tail\033[0m\n");
}

candy_pack_t *candy_object_search(candy_object_t obj, candy_hash_t hash){
  candy_assert(obj != NULL);
  candy_pack_t *temp = (candy_pack_t *)candy_get_next(obj);
  while (temp != NULL){
    if (*temp == NULL)
      return NULL;
    else if (candy_pack_match(*temp, hash))
      break;
    temp = (candy_pack_t *)candy_get_next(*temp);
  }
  return temp;
}

int candy_object_register(candy_object_t obj, candy_register_t table[]){
  candy_assert(obj != NULL);
  candy_assert(table != NULL);
  for (; table->name != NULL && table->method != NULL; table++)
    candy_object_push_method(obj, candy_time33(table->name), table->method);
  return 0;
}

candy_string_t candy_object_get_string(candy_object_t obj, candy_hash_t hash){
  candy_assert(obj != NULL);
  candy_pack_t *pack = candy_object_search(obj, hash);
  return (pack == NULL) ? (candy_string_t){NULL, 0} : candy_pack_get_string(*pack);
}

candy_integer_t candy_object_get_integer(candy_object_t obj, candy_hash_t hash){
  candy_assert(obj != NULL);
  candy_pack_t *pack = candy_object_search(obj, hash);
  return (pack == NULL) ? 0 : candy_pack_get_integer(*pack);
}

candy_float_t candy_object_get_float(candy_object_t obj, candy_hash_t hash){
  candy_assert(obj != NULL);
  candy_pack_t *pack = candy_object_search(obj, hash);
  return (pack == NULL) ? 0.0f : candy_pack_get_float(*pack);
}

candy_method_t candy_object_get_method(candy_object_t obj, candy_hash_t hash){
  candy_assert(obj != NULL);
  candy_pack_t *pack = candy_object_search(obj, hash);
  return (pack == NULL) ? NULL : candy_pack_get_method(*pack);
}

candy_object_t candy_object_get_object(candy_object_t obj, candy_hash_t hash){
  candy_assert(obj != NULL);
  candy_pack_t *pack = candy_object_search(obj, hash);
  return (pack == NULL) ? NULL : candy_pack_get_object(*pack);
}
