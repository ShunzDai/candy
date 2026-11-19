/**
  * Copyright 2022-2026 ShunzDai
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
#include "core/candy_wrap.h"
#include "core/candy_lib.h"
#include "core/candy_gc.h"
#include <string.h>

const candy_wrap_t CANDY_WRAP_NULL = { .type = CANDY_TYPE_NULL, .mask = MASK_NONE };

candy_err_t candy_wrap_mark(const candy_wrap_t *self, candy_gc_t *gc) {
  if (!(candy_wrap_mask(self) & MASK_OBJECT))
    return CANDY_OK;
  return candy_gc_mark(gc, candy_wrap_get_object(self));
}

CANDY_ALWAYS_INLINE candy_hash_t candy_wrap_hash(const candy_wrap_t *self) {
  uint8_t mask = candy_wrap_mask(self);
  candy_hash_t hash;
  if (mask & MASK_ARRAY) {
    /* Arrays that hash by value (interned strings) cache their content hash in
     * the object header; there is nothing to dispatch. */
    hash = candy_object_hash(candy_wrap_get_object(self));
  }
  else if (mask & MASK_OBJECT) {
    hash = (candy_hash_t)((uintptr_t)candy_wrap_get_object(self) >> 3);
  }
  else {
    switch (candy_wrap_type(self)) {
      case CANDY_TYPE_INTEGER:
      case CANDY_TYPE_FLOAT: {
        uint64_t bits;
        memcpy(&bits, candy_wrap_data(self), sizeof(bits));
        hash = (candy_hash_t)((bits >> 32) ^ bits);
        break;
      }
      case CANDY_TYPE_BOOLEAN:
        hash = candy_wrap_get_boolean(self) ? 1 : 0;
        break;
      case CANDY_TYPE_CFUNC:
        hash = (candy_hash_t)((uintptr_t)candy_wrap_get_cfunc(self) >> 3);
        break;
      default:
        hash = 0;
        break;
    }
  }
  return hash;
}

CANDY_ALWAYS_INLINE bool candy_wrap_equal(const candy_wrap_t *self, const candy_wrap_t *other) {
  candy_types_t type = candy_wrap_type(self);
  if (type != candy_wrap_type(other))
    return false;
  switch (type) {
    case CANDY_TYPE_NULL:
    case CANDY_TYPE_NONE:
      return true;
    case CANDY_TYPE_INTEGER:
      return candy_wrap_get_integer(self) == candy_wrap_get_integer(other);
    case CANDY_TYPE_FLOAT:
      return candy_wrap_get_float(self) == candy_wrap_get_float(other);
    case CANDY_TYPE_BOOLEAN:
      return candy_wrap_get_boolean(self) == candy_wrap_get_boolean(other);
    case CANDY_TYPE_CFUNC:
      return candy_wrap_get_cfunc(self) == candy_wrap_get_cfunc(other);
    default:
      /* Every string the script can hold is interned, so equal content means
       * equal address: object equality is identity. */
      return candy_wrap_get_object(self) == candy_wrap_get_object(other);
  }
}

int candy_wrap_fprint(const candy_wrap_t *self, candy_gc_t *gc, FILE *out) {
  const candy_format_t fmt = {.out = out, .quoted = false, .level = 0};
  return candy_wrap_format(self, gc, &fmt);
}

int candy_wrap_format(const candy_wrap_t *self, candy_gc_t *gc, const candy_format_t *fmt) {
  FILE *out = fmt->out;
  int res = 0;
  if (candy_wrap_mask(self) & MASK_OBJECT) {
    res = candy_gc_event_handler(gc)(candy_wrap_get_object(self), gc, EVT_FORMAT, (void *)fmt);
  }
  else {
    switch (candy_wrap_type(self)) {
      case CANDY_TYPE_NULL:
        res = fprintf(out, "%s", "null");
        break;
      case CANDY_TYPE_NONE:
        res = fprintf(out, "%s", "none");
        break;
      case CANDY_TYPE_INTEGER:
        res = fprintf(out, "%" PRId64, candy_wrap_get_integer(self));
        break;
      case CANDY_TYPE_FLOAT:
        res = fprintf(out, "%g", candy_wrap_get_float(self));
        break;
      case CANDY_TYPE_CHAR:
        res = fprintf(out, "%p", candy_wrap_get_object(self));
        break;
      case CANDY_TYPE_BOOLEAN:
        res = fprintf(out, "%s", candy_wrap_get_boolean(self) ? "true" : "false");
        break;
      default:
        res = fprintf(out, "%s", "NA");
        break;
    }
  }
  return res;
}
