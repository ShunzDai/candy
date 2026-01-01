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
#include "core/candy_binary.h"
#include "core/candy_exception.h"

static void _entry(void *self) {

}

candy_err_t candy_load(candy_gc_t *gc, candy_excep_t *ctx, candy_reader_t reader, void *arg, candy_object_t **out) {
  candy_err_t err = candy_excep_try(ctx, (candy_excep_cb_t)_entry, NULL, out);
  return err;
}
