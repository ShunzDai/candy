/**
  * Copyright 2022 ShunzDai
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
#ifndef CANDY_SRC_CORE_VM_H
#define CANDY_SRC_CORE_VM_H
#ifdef __cplusplus
extern "C"{
#endif /* __cplusplus */

#include "src/common/candy_types.h"

typedef struct candy_vm * candy_vm_t;

int candy_vm_load(candy_vm_t vm, const uint8_t binary[]);
int candy_vm_execute(candy_vm_t vm);

#ifdef __cplusplus
}
#endif /* __cplusplus */
#endif /* CANDY_SRC_CORE_VM_H */
