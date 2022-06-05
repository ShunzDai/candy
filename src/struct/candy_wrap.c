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
#include "src/platform/candy_memory.h"

#define CANDY_WRAP_TYPE  4
#define CANDY_WRAP_SZIE 16
#define CANDY_WRAP_RSVD 12

#if (CANDY_WRAP_TYPE + CANDY_WRAP_SZIE + CANDY_WRAP_RSVD) != 32
#error "private common struct size error"
#endif

typedef struct priv{
  uint32_t type : CANDY_WRAP_TYPE;
  uint32_t size : CANDY_WRAP_SZIE;
  uint32_t      : CANDY_WRAP_RSVD;
  candy_hash_t hash;
  uint8_t data[];
} * priv_t;

static inline priv_t _private(candy_wrap_t wrap){
  return (priv_t)(((struct {candy_wrap_t next; uint8_t args[];} *)wrap)->args);
}

inline candy_span_t candy_wrap_expand(candy_wrap_t wrap){
  candy_assert(wrap != NULL);
  return (candy_span_t){_private(wrap)->data, _private(wrap)->size};
}

inline candy_wraps_t candy_wrap_type(candy_wrap_t wrap){
  candy_assert(wrap != NULL);
  return _private(wrap)->type;
}

inline bool candy_wrap_match(candy_wrap_t wrap, candy_hash_t hash){
  candy_assert(wrap != NULL);
  return (_private(wrap)->hash == hash);
}

static candy_wrap_t _candy_wrap_create(candy_hash_t hash, candy_span_t span, candy_wraps_t type, candy_wrap_t next){
  candy_wrap_t wrap = (candy_wrap_t)candy_malloc(sizeof(struct candy_wrap) + sizeof(struct priv) + span.size);
  wrap->next = next;
  _private(wrap)->size = span.size;
  _private(wrap)->type = type;
  _private(wrap)->hash = hash;
  (span.data == NULL) ? candy_memset(_private(wrap)->data, 0, span.size) : candy_memcpy(_private(wrap)->data, span.data, span.size);
  return wrap;
}

candy_wrap_t candy_wrap_delete(candy_wrap_t wrap){
  candy_wrap_t temp = wrap->next;
  candy_free(wrap);
  wrap = NULL;
  return temp;
}

inline candy_wrap_t candy_wrap_copy(candy_wrap_t wrap){
  candy_assert(wrap != NULL);
  return _candy_wrap_create(_private(wrap)->hash, (candy_span_t){_private(wrap)->data, _private(wrap)->size}, _private(wrap)->type, NULL);
}

inline candy_wrap_t candy_wrap_none(candy_hash_t hash){
  return _candy_wrap_create(hash, (candy_span_t){NULL, 0}, CANDY_WRAP_NONE, NULL);
}

inline candy_wrap_t candy_wrap_integer(candy_hash_t hash, candy_integer_t value){
  return _candy_wrap_create(hash, (candy_span_t){&value, sizeof(candy_integer_t)}, CANDY_WRAP_INTEGER, NULL);
}

inline candy_wrap_t candy_wrap_float(candy_hash_t hash, candy_float_t value){
  return _candy_wrap_create(hash, (candy_span_t){&value, sizeof(candy_float_t)}, CANDY_WRAP_FLOAT, NULL);
}

inline candy_wrap_t candy_wrap_boolean(candy_hash_t hash, candy_boolean_t value){
  return _candy_wrap_create(hash, (candy_span_t){&value, sizeof(candy_boolean_t)}, CANDY_WRAP_BOOLEAN, NULL);
}

inline candy_wrap_t candy_wrap_method(candy_hash_t hash, candy_method_t value){
  return _candy_wrap_create(hash, (candy_span_t){&value, sizeof(candy_method_t)}, CANDY_WRAP_METHOD, NULL);
}

inline candy_wrap_t candy_wrap_string(candy_hash_t hash, candy_string_t value){
  return _candy_wrap_create(hash, (candy_span_t){value.data, value.size}, CANDY_WRAP_STRING, NULL);
}

inline candy_wrap_t candy_wrap_meta(candy_hash_t hash, candy_span_t span, candy_wraps_t type){
  return _candy_wrap_create(hash, span, type, NULL);
}

candy_wrap_t candy_wrap_set_none(candy_wrap_t wrap){
  candy_assert(wrap != NULL);
  switch (_private(wrap)->type){
    case CANDY_WRAP_NONE:
      return wrap;
    default: {
      candy_wrap_t next = wrap->next;
      candy_hash_t hash = _private(wrap)->hash;
      candy_free(wrap);
      return _candy_wrap_create(hash, (candy_span_t){NULL, 0}, CANDY_WRAP_NONE, next);
    }
  }
}

candy_wrap_t candy_wrap_set_integer(candy_wrap_t wrap, candy_integer_t value){
  candy_assert(wrap != NULL);
  switch (_private(wrap)->type){
    case CANDY_WRAP_INTEGER:
      *((candy_integer_t *)_private(wrap)->data) = value;
      return wrap;
    default: {
      candy_wrap_t next = wrap->next;
      candy_hash_t hash = _private(wrap)->hash;
      candy_free(wrap);
      return _candy_wrap_create(hash, (candy_span_t){&value, sizeof(candy_integer_t)}, CANDY_WRAP_INTEGER, next);
    }
  }
}

candy_wrap_t candy_wrap_set_float(candy_wrap_t wrap, candy_float_t value){
  candy_assert(wrap != NULL);
  switch (_private(wrap)->type){
    case CANDY_WRAP_FLOAT:
      *((candy_float_t *)_private(wrap)->data) = value;
      return wrap;
    default:{
      candy_wrap_t next = wrap->next;
      candy_hash_t hash = _private(wrap)->hash;
      candy_free(wrap);
      return _candy_wrap_create(hash, (candy_span_t){&value, sizeof(candy_float_t)}, CANDY_WRAP_FLOAT, next);
    }
  }
}

candy_wrap_t candy_wrap_set_boolean(candy_wrap_t wrap, candy_boolean_t value){/* if value % 256 == 0 ? */
  candy_assert(wrap != NULL);
  switch (_private(wrap)->type){
    case CANDY_WRAP_BOOLEAN:
      *((candy_boolean_t *)_private(wrap)->data) = value;
      return wrap;
    default: {
      candy_wrap_t next = wrap->next;
      candy_hash_t hash = _private(wrap)->hash;
      candy_free(wrap);
      return _candy_wrap_create(hash, (candy_span_t){&value, sizeof(candy_boolean_t)}, CANDY_WRAP_BOOLEAN, next);
    }
  }
}

candy_wrap_t candy_wrap_set_method(candy_wrap_t wrap, candy_method_t method){
  candy_assert(wrap != NULL);
  switch (_private(wrap)->type){
    case CANDY_WRAP_METHOD:
      *((candy_method_t *)_private(wrap)->data) = method;
      return wrap;
    default: {
      candy_wrap_t next = wrap->next;
      candy_hash_t hash = _private(wrap)->hash;
      candy_free(wrap);
      return _candy_wrap_create(hash, (candy_span_t){&method, sizeof(candy_method_t)}, CANDY_WRAP_METHOD, next);
    }
  }
}

candy_wrap_t candy_wrap_set_string(candy_wrap_t wrap, candy_string_t string){
  candy_assert(wrap != NULL);
  candy_wrap_t next = wrap->next;
  candy_hash_t hash = _private(wrap)->hash;
  candy_free(wrap);
  return _candy_wrap_create(hash, (candy_span_t){string.data, string.size}, CANDY_WRAP_STRING, next);
}

candy_integer_t candy_wrap_get_integer(candy_wrap_t wrap){
#define _round(x) ((x) >= 0 ? (candy_integer_t)((x) + 0.5) : (candy_integer_t)((x) - 0.5))
  candy_assert(wrap != NULL);
  switch (_private(wrap)->type){
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

candy_float_t candy_wrap_get_float(candy_wrap_t wrap){
  candy_assert(wrap != NULL);
  switch (_private(wrap)->type){
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

candy_boolean_t candy_wrap_get_boolean(candy_wrap_t wrap){
  candy_assert(wrap != NULL);
  switch (_private(wrap)->type){
    case CANDY_WRAP_BOOLEAN:
      return *(candy_boolean_t *)_private(wrap)->data;
    case CANDY_WRAP_INTEGER:
      return (candy_boolean_t)(*(candy_integer_t *)_private(wrap)->data);
    default:
      candy_assert(false);
      return -1;
  }
}

candy_method_t candy_wrap_get_method(candy_wrap_t wrap){
  candy_assert(wrap != NULL);
  switch (_private(wrap)->type){
    case CANDY_WRAP_METHOD:
      return *(candy_method_t *)_private(wrap)->data;
    default:
      candy_assert(false);
      return NULL;
  }
}

candy_string_t candy_wrap_get_string(candy_wrap_t wrap){
  candy_assert(wrap != NULL);
  switch (_private(wrap)->type){
    case CANDY_WRAP_STRING:
      return (candy_string_t){_private(wrap)->data, _private(wrap)->size};
    default:
      candy_assert(false);
      return (candy_string_t){NULL, 0};
  }
}
