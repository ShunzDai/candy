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
#ifndef CANDY_SRC_PLATFORM_LIB_H
#define CANDY_SRC_PLATFORM_LIB_H
#ifdef __cplusplus
extern "C"{
#endif /* __cplusplus */

#include "src/common/candy_types.h"

candy_hash_t candy_hash(char *name);

void *candy_memset(void *dst, uint8_t val, size_t size);
void *candy_memcpy(void *dst, const void *src, size_t size);

#ifdef __cplusplus
}
#endif /* __cplusplus */
#endif /* CANDY_SRC_PLATFORM_LIB_H */
