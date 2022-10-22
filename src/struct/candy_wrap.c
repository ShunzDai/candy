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
#include "candy_wrap.h"
#include "src/common/candy_lib.h"
#include <stdlib.h>

typedef struct priv {
  candy_hash_t hash;
  uint16_t type : 4;
  uint16_t      : 12;
  uint16_t size;
  uint8_t data[];
} * priv_t;

static inline priv_t _private(candy_wrap_t wrap) {
  return (priv_t)(((struct {candy_wrap_t next; uint8_t args[];} *)wrap)->args);
}

candy_wraps_t candy_wrap_print(candy_wrap_t wrap) {
  switch (_private(wrap)->type) {
    case CANDY_WRAP_NONE:
      printf("\n");
      break;
    case CANDY_WRAP_STRING:
      printf("%s\n", (char *)candy_wrap_get_string(wrap)->data);
      break;
    case CANDY_WRAP_INTEGER:
      printf("%d\n", candy_wrap_get_integer(wrap));
      return CANDY_WRAP_INTEGER;
    case CANDY_WRAP_FLOAT:
      printf("%.5f\n", candy_wrap_get_float(wrap));
      break;
    case CANDY_WRAP_BOOLEAN:
      printf("%s\n", candy_wrap_get_boolean(wrap) ? "True" : "False");
      break;
    case CANDY_WRAP_METHOD:
      printf("%p\n", candy_wrap_get_method(wrap));
      break;
    case CANDY_WRAP_OBJECT:
      printf("%p\n", wrap);
      break;
    default:
      printf("exception types");
      break;
  }
  return (candy_wraps_t)_private(wrap)->type;
}

inline candy_view_t candy_wrap_view(candy_wrap_t wrap) {
  candy_assert(wrap != NULL);
  return (candy_view_t)&_private(wrap)->size;
}

inline candy_wraps_t candy_wrap_type(candy_wrap_t wrap) {
  candy_assert(wrap != NULL);
  return (candy_wraps_t)_private(wrap)->type;
}

inline bool candy_wrap_match(candy_wrap_t wrap, candy_hash_t hash) {
  candy_assert(wrap != NULL);
  return (_private(wrap)->hash == hash);
}

candy_wrap_t candy_wrap_create(candy_hash_t hash, const void *data, uint16_t size, candy_wraps_t type, candy_wrap_t next) {
  candy_wrap_t wrap = (candy_wrap_t)malloc(sizeof(struct candy_wrap) + sizeof(struct priv) + size);
  wrap->next = next;
  _private(wrap)->size = size;
  _private(wrap)->type = type;
  _private(wrap)->hash = hash;
  (data == NULL) ? candy_memset(_private(wrap)->data, 0, size) : candy_memcpy(_private(wrap)->data, data, size);
  return wrap;
}

int candy_wrap_delete(candy_wrap_t *wrap) {
  candy_wrap_t temp = (*wrap)->next;
  free(*wrap);
  *wrap = temp;
  return 0;
}

inline candy_wrap_t candy_wrap_copy(candy_wrap_t wrap) {
  candy_assert(wrap != NULL);
  return candy_wrap_create(_private(wrap)->hash, _private(wrap)->data, _private(wrap)->size, (candy_wraps_t)_private(wrap)->type, NULL);
}

int candy_wrap_set_none(candy_wrap_t *wrap) {
  candy_assert(wrap != NULL);
  switch (_private(*wrap)->type) {
    case CANDY_WRAP_NONE:
      return 0;
    default: {
      candy_wrap_t next = (*wrap)->next;
      candy_hash_t hash = _private(*wrap)->hash;
      free(*wrap);
      *wrap = candy_wrap_create(hash, NULL, 0, CANDY_WRAP_NONE, next);
      return 0;
    }
  }
}

int candy_wrap_set_integer(candy_wrap_t *wrap, candy_integer_t value) {
  candy_assert(wrap != NULL);
  switch (_private(*wrap)->type) {
    case CANDY_WRAP_INTEGER:
      *((candy_integer_t *)_private(*wrap)->data) = value;
      return 0;
    default: {
      candy_wrap_t next = (*wrap)->next;
      candy_hash_t hash = _private(*wrap)->hash;
      free(*wrap);
      *wrap = candy_wrap_create(hash, &value, sizeof(candy_integer_t), CANDY_WRAP_INTEGER, next);
      return 0;
    }
  }
}

int candy_wrap_set_float(candy_wrap_t *wrap, candy_float_t value) {
  candy_assert(wrap != NULL);
  switch (_private(*wrap)->type) {
    case CANDY_WRAP_FLOAT:
      *((candy_float_t *)_private(*wrap)->data) = value;
      return 0;
    default:{
      candy_wrap_t next = (*wrap)->next;
      candy_hash_t hash = _private(*wrap)->hash;
      free(*wrap);
      *wrap = candy_wrap_create(hash, &value, sizeof(candy_float_t), CANDY_WRAP_FLOAT, next);
      return 0;
    }
  }
}

int candy_wrap_set_boolean(candy_wrap_t *wrap, candy_boolean_t value){/* if value % 256 == 0 ? */
  candy_assert(wrap != NULL);
  switch (_private(*wrap)->type) {
    case CANDY_WRAP_BOOLEAN:
      *((candy_boolean_t *)_private(*wrap)->data) = value;
      return 0;
    default: {
      candy_wrap_t next = (*wrap)->next;
      candy_hash_t hash = _private(*wrap)->hash;
      free(*wrap);
      *wrap = candy_wrap_create(hash, &value, sizeof(candy_boolean_t), CANDY_WRAP_BOOLEAN, next);
      return 0;
    }
  }
}

int candy_wrap_set_method(candy_wrap_t *wrap, candy_method_t value) {
  candy_assert(wrap != NULL);
  switch (_private(*wrap)->type) {
    case CANDY_WRAP_METHOD:
      *((candy_method_t *)_private(*wrap)->data) = value;
      return 0;
    default: {
      candy_wrap_t next = (*wrap)->next;
      candy_hash_t hash = _private(*wrap)->hash;
      free(*wrap);
      *wrap = candy_wrap_create(hash, &value, sizeof(candy_method_t), CANDY_WRAP_METHOD, next);
      return 0;
    }
  }
}

int candy_wrap_set_string(candy_wrap_t *wrap, const char value[], uint16_t size) {
  candy_assert(wrap != NULL);
  candy_wrap_t next = (*wrap)->next;
  candy_hash_t hash = _private(*wrap)->hash;
  free(*wrap);
  *wrap = candy_wrap_create(hash, value, size, CANDY_WRAP_STRING, next);
  return 0;
}

candy_integer_t candy_wrap_get_integer(candy_wrap_t wrap) {
#define _round(x) ((x) >= 0 ? (candy_integer_t)((x) + 0.5) : (candy_integer_t)((x) - 0.5))
  candy_assert(wrap != NULL);
  switch (_private(wrap)->type) {
    case CANDY_WRAP_INTEGER:
      return *(candy_integer_t *)_private(wrap)->data;
    case CANDY_WRAP_FLOAT:
      return _round(*(candy_float_t *)_private(wrap)->data);
    case CANDY_WRAP_BOOLEAN:
      return *(candy_boolean_t *)_private(wrap)->data ? 1 : 0;
    default:
      candy_assert(false);
      return -1;
  }
#undef _round
}

candy_float_t candy_wrap_get_float(candy_wrap_t wrap) {
  candy_assert(wrap != NULL);
  switch (_private(wrap)->type) {
    case CANDY_WRAP_FLOAT:
      return *(candy_float_t *)_private(wrap)->data;
    case CANDY_WRAP_INTEGER:
      return (candy_float_t)(*(candy_integer_t *)_private(wrap)->data);
    case CANDY_WRAP_BOOLEAN:
      return *(candy_boolean_t *)_private(wrap)->data ? 1.0f : 0.0f;
    default:
      candy_assert(false);
      return -1;
  }
}

candy_boolean_t candy_wrap_get_boolean(candy_wrap_t wrap) {
  candy_assert(wrap != NULL);
  switch (_private(wrap)->type) {
    case CANDY_WRAP_BOOLEAN:
      return *(candy_boolean_t *)_private(wrap)->data;
    case CANDY_WRAP_INTEGER:
      return (candy_boolean_t)(*(candy_integer_t *)_private(wrap)->data);
    default:
      candy_assert(false);
      return false;
  }
}

candy_method_t candy_wrap_get_method(candy_wrap_t wrap) {
  candy_assert(wrap != NULL);
  switch (_private(wrap)->type) {
    case CANDY_WRAP_METHOD:
      return *(candy_method_t *)_private(wrap)->data;
    default:
      candy_assert(false);
      return NULL;
  }
}

candy_string_t candy_wrap_get_string(candy_wrap_t wrap) {
  candy_assert(wrap != NULL);
  switch (_private(wrap)->type) {
    case CANDY_WRAP_STRING:
      return (candy_view_t)(&_private(wrap)->size);
    default:
      candy_assert(false);
      return (candy_view_t)NULL;
  }
}
