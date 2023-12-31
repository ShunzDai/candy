/**
  * Copyright 2022-2023 ShunzDai
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
#ifndef CANDY_SRC_VM_H
#define CANDY_SRC_VM_H
#ifdef __cplusplus
extern "C"{
#endif /* __cplusplus */

#include "src/candy_wrap.h"
#include "src/candy_types.h"

typedef struct candy_vm candy_vm_t;

struct candy_vm {
  candy_state_t *sta;
  candy_wrap_t base;
  size_t top;
  candy_table_t *glb;
};

int candy_vm_init(candy_vm_t *self, candy_state_t *sta);
int candy_vm_deinit(candy_vm_t *self);

int candy_vm_dump_global(candy_vm_t *self);

void candy_vm_push(candy_vm_t *self, const candy_wrap_t *wrap);
const candy_wrap_t *candy_vm_pop(candy_vm_t *self);

int candy_vm_builtin(candy_vm_t *self, candy_regist_t list[], size_t size);
int candy_vm_set_global(candy_vm_t *self, const char name[]);
int candy_vm_get_global(candy_vm_t *self, const char name[]);
int candy_vm_call(candy_vm_t *self, int nargs, int nresults);
int candy_vm_execute(candy_vm_t *self, candy_block_t *block);

#ifdef __cplusplus
}
#endif /* __cplusplus */
#endif /* CANDY_SRC_VM_H */
