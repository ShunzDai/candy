/**
  * Copyright 2022-2025 ShunzDai
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
#include "core/candy_exception.h"
#include "core/candy_gc.h"
#include <inttypes.h>

const candy_wrap_t CANDY_WRAP_NULL = {0};

candy_hash_t candy_wrap_hash(const candy_wrap_t *self, candy_gc_t *gc, candy_excep_t *ctx) {
  if ((candy_wrap_mask(self) & MASK_HASHABLE) == 0) {
    candy_excep_throw(ctx, CANDY_ERR_UNHASH, candy_wrap_get_object(self));
  }
  if (candy_wrap_mask(self) & MASK_ARRAY) {
    candy_hash_t hash;
    candy_gc_event_handler(gc)(candy_wrap_get_object(self), gc, EVT_HASH, &hash);
    return hash;
  }
  switch (candy_wrap_type(self)) {
    case CANDY_TYPE_NULL:
    case CANDY_TYPE_BOOLEAN:
    case CANDY_TYPE_INTEGER:
    case CANDY_TYPE_FLOAT:
    case CANDY_TYPE_CHAR:
      return *(candy_hash_t *)candy_wrap_data(self);
    default:
      return 0;
  }
}

int candy_wrap_fprint(const candy_wrap_t *self, FILE *out, int align) {
  switch (candy_wrap_type(self)) {
    case CANDY_TYPE_NULL:
      return fprintf(out, "%*s", align, "null");
    case CANDY_TYPE_INTEGER:
      return fprintf(out, "%*" PRId64, align, candy_wrap_get_integer(self));
    case CANDY_TYPE_FLOAT:
      return fprintf(out, "%*f", align, candy_wrap_get_float(self));
    case CANDY_TYPE_CHAR:
      return fprintf(out, "%*p", align, candy_wrap_get_object(self));
    default:
      return fprintf(out, "%*s", align, "NA");
  }
}
