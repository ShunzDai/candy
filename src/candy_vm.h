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

#include "src/candy_types.h"

typedef struct candy_vm candy_vm_t;

candy_vm_t *candy_vm_create(candy_state_t *sta);
int candy_vm_delete(candy_vm_t **self);

void candy_vm_regist_entry(candy_vm_t *self, candy_cfunc_t entry);

void candy_vm_push_integer(candy_vm_t *self, candy_integer_t *val, int size);
void candy_vm_push_float(candy_vm_t *self, candy_float_t *val, int size);
void candy_vm_push_boolean(candy_vm_t *self, candy_boolean_t *val, int size);
void candy_vm_push_string(candy_vm_t *self, char *val, int size);

candy_integer_t *candy_vm_pull_integer(candy_vm_t *self, int *size);
candy_float_t *candy_vm_pull_float(candy_vm_t *self, int *size);
candy_boolean_t *candy_vm_pull_boolean(candy_vm_t *self, int *size);
char *candy_vm_pull_string(candy_vm_t *self, int *size);

int candy_vm_load(candy_vm_t *self, const uint8_t binary[]);
int candy_vm_execute(candy_vm_t *self);

#ifdef __cplusplus
}
#endif /* __cplusplus */
#endif /* CANDY_SRC_VM_H */
