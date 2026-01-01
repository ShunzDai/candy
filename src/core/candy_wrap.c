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

const candy_wrap_t CANDY_WRAP_NULL = {0};

candy_hash_t candy_wrap_hash(const candy_wrap_t *self, candy_gc_t *gc) {
  candy_hash_t hash = 0;
  if (candy_wrap_mask(self) & MASK_OBJECT) {
    candy_gc_event_handler(gc)(candy_wrap_get_object(self), gc, EVT_HASH, &hash);
  }
  else {
    hash = *(candy_hash_t *)candy_wrap_data(self);
  }
  return hash;
}

candy_err_t candy_wrap_fprint(const candy_wrap_t *self, candy_gc_t *gc, FILE *out) {
  if (candy_wrap_mask(self) & MASK_OBJECT) {
    candy_gc_event_handler(gc)(candy_wrap_get_object(self), gc, EVT_FORMAT, out);
  }
  else {
    switch (candy_wrap_type(self)) {
      case CANDY_TYPE_NULL:
        fprintf(out, "%s", "null");
        break;
      case CANDY_TYPE_INTEGER:
        fprintf(out, "%" PRId64, candy_wrap_get_integer(self));
        break;
      case CANDY_TYPE_FLOAT:
        fprintf(out, "%f", candy_wrap_get_float(self));
        break;
      case CANDY_TYPE_CHAR:
        fprintf(out, "%p", candy_wrap_get_object(self));
        break;
      case CANDY_TYPE_BOOLEAN:
        fprintf(out, "%s", candy_wrap_get_boolean(self) ? "true" : "false");
        break;
      default:
        fprintf(out, "%s", "NA");
        break;
    }
  }
  return CANDY_OK;
}
