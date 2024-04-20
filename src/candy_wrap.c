/**
  * Copyright 2022-2024 ShunzDai
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
#include "src/candy_wrap.h"
#include "src/candy_lib.h"
#include <inttypes.h>

int candy_wrap_fprint(const candy_wrap_t *self, FILE *out, int align) {
  switch (candy_wrap_get_type(self)) {
    case TYPE_NONE:
      return fprintf(out, "%*s", align, "NONE");
    case TYPE_INTEGER:
      return fprintf(out, "%*" PRId64, align, candy_wrap_get_integer(self));
    case TYPE_FLOAT:
      return fprintf(out, "%*f", align, candy_wrap_get_float(self));
    // case TYPE_CFUNC:
    //   return fprintf(out, "%*p", align, candy_wrap_get_cfunc(self));
    default:
      return fprintf(out, "%*s", align, "NA");
  }
}
