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
#include "candy_utility.h"
#include "core/candy_vm.h"
#include <stdlib.h>

static int _exit(candy_state_t *self) {
  exit(0);
  return 0;
}

static int _print(candy_state_t *self) {

  return 0;
}

const candy_regist_t candy_builtin_list_utility[] = {
  {"exit" ,  _exit},
  {"print", _print},
  {   NULL,   NULL},
};
