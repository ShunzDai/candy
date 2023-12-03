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
#ifndef CANDY_SRC_OBJECT_H
#define CANDY_SRC_OBJECT_H
#ifdef __cplusplus
extern "C"{
#endif /* __cplusplus */

#include "src/candy_wrap.h"
#include "src/candy_types.h"

struct candy_array {
  candy_wrap_t obj;
  size_t size;
  void *data;
};

struct candy_block {
  candy_wrap_t obj;
  candy_array_t *pool;
  candy_array_t *ins;
};

struct candy_cclosure {
  candy_wrap_t obj;
  candy_block_t *block;
};

struct candy_sclosure {
  candy_wrap_t obj;
  candy_block_t *block;
};

#ifdef __cplusplus
}
#endif /* __cplusplus */
#endif /* CANDY_SRC_OBJECT_H */
