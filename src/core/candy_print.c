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
#include "core/candy_print.h"
#include "core/candy_gc.h"
#include "core/candy_array.h"
#include <stdio.h>

candy_array_t *candy_vprint(candy_gc_t *gc, candy_exce_t *ctx, const char format[], va_list args) {
  candy_array_t *out = candy_array_create(gc, ctx, CANDY_BASE_CHAR, MASK_NONE);
  va_list args_copy;
  va_copy(args_copy, args);
  int len = vsnprintf(NULL, 0, format, args_copy);
  if (len < 0)
    return out;
	candy_array_resize(out, gc, ctx, len + 1);
  vsnprintf((char*)candy_array_data(out), candy_array_capacity(out), format, args);
  candy_array_resize(out, gc, ctx, len);
  return out;
}

candy_array_t *candy_print(candy_gc_t *gc, candy_exce_t *ctx, const char format[], ...) {
  va_list args;
	va_start(args, format);
	candy_array_t *out = candy_vprint(gc, ctx, format, args);
	va_end(args);
  return out;
}
