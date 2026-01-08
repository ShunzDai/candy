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
#include "candy_utils.h"
#include "core/candy_state.h"
#include <stdlib.h>

static int _builtin_exit(candy_state_t *self) {
  exit(0);
  return 0;
}

static int _builtin_print(candy_state_t *self) {
  candy_state_fprint(self, 0, stdout);
  fprintf(stdout, "\n");
  fflush(stdout);
  return 0;
}

candy_err_t candy_builtin_entry_utils(candy_state_t *self) {
  candy_state_setglobal(self, "exit", _builtin_exit);
  candy_state_setglobal(self, "print", _builtin_print);
  return CANDY_OK;
}
