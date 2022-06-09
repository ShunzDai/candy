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
#include "src/struct/candy_wrap.h"
#include "src/struct/candy_queue.h"

static const char * const _wraps_string[] = {
  "none",
  "integer",
  "float",
  "boolean",
  "string",
  "method",
  "object",
  "unknown"
};

static inline candy_queue_t _private(candy_object_t obj){
  return (candy_queue_t)candy_wrap_expand(obj).data;
}

static void _print_with_depth(candy_object_t obj, uint32_t depth){
  candy_wrap_t temp = (candy_wrap_t)_private(obj)->next;
  uint32_t count = 0;
  char offset[depth + 1];
  memset(offset, '\t', depth);
  offset[depth] = '\0';
  printf("%s\033[1;35m>>> %sobject head\033[0m\n", offset, depth ? "sub " : "");
  printf("%sdepth-pos\taddress\t\ttype\tsize\tvalue\n", offset);
  while (temp != NULL){
    printf("%s[%d-%d]\t\t%p\t%s\t%d\t", offset, depth, count, temp, _wraps_string[candy_wrap_type(temp) < CANDY_WRAP_MAX ? candy_wrap_type(temp) : CANDY_WRAP_MAX], candy_wrap_expand(temp).size);
    if (candy_wrap_print(temp) == CANDY_WRAP_OBJECT)
      _print_with_depth(temp, depth + 1);
    count++;
    temp = temp->next;
  }
  printf("%sdepth: %d, count(s): %d\n",offset, depth, count);
  printf("%s\033[1;35m<<< %sobject tail\033[0m\n", offset, depth ? "sub " : "");
}

int candy_object_print(candy_object_t obj){
  candy_assert(obj != NULL);
  _print_with_depth(obj, 0);
  return 0;
}

candy_wrap_t *candy_object_search(candy_object_t obj, candy_hash_t hash){
  candy_assert(obj != NULL);
  candy_wrap_t *temp = (candy_wrap_t *)&_private(obj)->next;
  while (temp != NULL){
    if (*temp == NULL)
      return temp;
    else if (candy_wrap_match(*temp, hash))
      break;
    temp = &(*temp)->next;
  }
  return temp;
}

// int candy_object_register(candy_object_t obj, candy_register_t table[]){
//   candy_assert(obj != NULL);
//   candy_assert(table != NULL);
//   for (; table->name != NULL && table->method != NULL; table++)
//     candy_object_push_method(obj, candy_time33(table->name), table->method);
//   return 0;
// }

candy_object_t candy_object_create(candy_hash_t hash){
  return candy_wrap_meta(hash, (candy_span_t){NULL, candy_queue_size()}, CANDY_WRAP_OBJECT);
}

candy_object_t candy_object_delete(candy_object_t obj){
  if (candy_wrap_type(obj) == CANDY_WRAP_OBJECT){
    candy_queue_clear(_private(obj), (candy_destroy_t)candy_object_delete);
    candy_assert(*(candy_queue_t *)candy_wrap_expand(obj).data == NULL);
  }
  return (candy_object_t)candy_wrap_delete((candy_wrap_t)obj);
}

int candy_object_push(candy_object_t obj, candy_wrap_t wrap){
  candy_assert(obj != NULL);
  candy_assert(candy_wrap_type(obj) == CANDY_WRAP_OBJECT);
  return candy_enqueue(_private(obj), 0, (candy_node_t)wrap);
}

int candy_object_push_none(candy_object_t obj, candy_hash_t hash){
  return candy_object_push(obj, candy_wrap_none(hash));
}

int candy_object_push_integer(candy_object_t obj, candy_hash_t hash, candy_integer_t value){
  return candy_object_push(obj, candy_wrap_integer(hash, value));
}

int candy_object_push_float(candy_object_t obj, candy_hash_t hash, candy_float_t value){
  return candy_object_push(obj, candy_wrap_float(hash, value));
}

int candy_object_push_method(candy_object_t obj, candy_hash_t hash, candy_method_t value){
  return candy_object_push(obj, candy_wrap_method(hash, value));
}

int candy_object_push_string(candy_object_t obj, candy_hash_t hash, candy_string_t value){
  return candy_object_push(obj, candy_wrap_string(hash, value));
}

int candy_object_push_object(candy_object_t obj, candy_hash_t hash){
  return candy_object_push(obj, candy_object_create(hash));
}

int candy_object_pop(candy_object_t obj, candy_hash_t hash){
  candy_assert(obj != NULL);
  candy_wrap_t *wrap = candy_object_search(obj, hash);
  if (wrap == NULL)
    return -1;
  candy_wrap_t del = *wrap;
  *wrap = (*wrap)->next;
  candy_object_delete(del);
  return 0;
}

candy_string_t candy_object_get_string(candy_object_t obj, candy_hash_t hash){
  candy_assert(obj != NULL);
  candy_wrap_t *wrap = candy_object_search(obj, hash);
  return (wrap == NULL) ? (candy_string_t){NULL, 0} : candy_wrap_get_string(*wrap);
}

candy_integer_t candy_object_get_integer(candy_object_t obj, candy_hash_t hash){
  candy_assert(obj != NULL);
  candy_wrap_t *wrap = candy_object_search(obj, hash);
  return (wrap == NULL) ? 0 : candy_wrap_get_integer(*wrap);
}

candy_float_t candy_object_get_float(candy_object_t obj, candy_hash_t hash){
  candy_assert(obj != NULL);
  candy_wrap_t *wrap = candy_object_search(obj, hash);
  return (wrap == NULL) ? 0.0f : candy_wrap_get_float(*wrap);
}

candy_method_t candy_object_get_method(candy_object_t obj, candy_hash_t hash){
  candy_assert(obj != NULL);
  candy_wrap_t *wrap = candy_object_search(obj, hash);
  return (wrap == NULL) ? NULL : candy_wrap_get_method(*wrap);
}

candy_object_t candy_object_get_object(candy_object_t obj, candy_hash_t hash){
  candy_assert(obj != NULL);
  candy_wrap_t *wrap = candy_object_search(obj, hash);
  return (wrap == NULL) ? NULL : *wrap;
}
