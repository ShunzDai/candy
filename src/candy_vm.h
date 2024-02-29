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
extern "C" {
#endif /* __cplusplus */

#include "src/candy_wrap.h"
#include "src/candy_io.h"
#include "src/candy_types.h"

typedef struct candy_vm candy_vm_t;

struct candy_vm {
  /* global input-output buffer */
  candy_io_t io;
  candy_wrap_t glb;
  candy_wrap_t base;
  int top;
};

int candy_vm_init(candy_vm_t *self);
int candy_vm_deinit(candy_vm_t *self);

int candy_vm_fprint(candy_vm_t *self, FILE *out);

void candy_vm_push(candy_vm_t *self, const candy_wrap_t *wrap);
const candy_wrap_t *candy_vm_pop(candy_vm_t *self);

int candy_vm_regist(candy_vm_t *self, const candy_regist_t list[], int size);
int candy_vm_set_global(candy_vm_t *self, const char name[]);
int candy_vm_get_global(candy_vm_t *self, const char name[]);
int candy_vm_call(candy_vm_t *self, int nargs, int nresults);
int candy_vm_execute(candy_vm_t *self, candy_block_t *block);

#ifdef __cplusplus
}
#endif /* __cplusplus */
#endif /* CANDY_SRC_VM_H */
