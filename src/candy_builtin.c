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
#include "src/candy_builtin.h"
#include "src/candy_vm.h"
#include "src/candy_lib.h"
#include <stdlib.h>

int candy_builtin_exit(candy_state_t *self) {
  exit(0);
  return 0;
}

int candy_builtin_print(candy_state_t *self) {
  candy_vm_t *vm = (candy_vm_t *)self;
  candy_vm_fprint(vm, stdout);
  fprintf(stdout, "\n");
  return 0;
}

const candy_regist_t candy_builtin_list[] = {
  {"exit" , candy_builtin_exit },
  {"print", candy_builtin_print},
};

const int candy_builtin_size = candy_lengthof(candy_builtin_list);
