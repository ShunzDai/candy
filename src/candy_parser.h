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
#ifndef CANDY_SRC_LEXER_H
#define CANDY_SRC_LEXER_H
#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#include "src/candy_priv.h"

candy_object_t *candy_parse(candy_gc_t *gc, candy_reader_t reader, void *arg);

#ifdef __cplusplus
}
#endif /* __cplusplus */
#endif /* CANDY_SRC_LEXER_H */
