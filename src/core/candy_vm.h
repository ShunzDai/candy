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
#ifndef CANDY_CORE_VM_H
#define CANDY_CORE_VM_H
#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#include "core/candy_exception.h"
#include "core/candy_vector.h"
#include "core/candy_priv.h"

typedef struct candy_vm candy_vm_t;

struct candy_vm {
  candy_exce_t ctx;
  candy_vector_t root;
};

int candy_vm_init(candy_vm_t *self);
int candy_vm_deinit(candy_vm_t *self, candy_gc_t *gc);

int candy_vm_fprint(candy_vm_t *self, FILE *out);

void candy_vm_push(candy_vm_t *self, const candy_wrap_t *wrap);
const candy_wrap_t *candy_vm_pop(candy_vm_t *self);

int candy_vm_set_global(candy_vm_t *self, const char name[]);
int candy_vm_get_global(candy_vm_t *self, const char name[]);
int candy_vm_call(candy_vm_t *self, int nargs, int nresults);
int candy_vm_execute(candy_vm_t *self, candy_gc_t *gc);

#ifdef __cplusplus
}
#endif /* __cplusplus */
#endif /* CANDY_CORE_VM_H */
