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
#ifndef CANDY_SRC_TABLE_H
#define CANDY_SRC_TABLE_H
#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#include "src/candy_types.h"

int candy_table_fprint(const candy_wrap_t *self, FILE *out);
const candy_wrap_t *candy_table_get(candy_wrap_t *self, const candy_wrap_t *key);
int candy_table_set(candy_wrap_t *self, const candy_wrap_t *key, const candy_wrap_t *val);

#ifdef __cplusplus
}
#endif /* __cplusplus */
#endif /* CANDY_SRC_TABLE_H */
