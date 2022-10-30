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
#ifndef CANDY_SRC_LIB_H
#define CANDY_SRC_LIB_H
#ifdef __cplusplus
extern "C"{
#endif /* __cplusplus */

#include "src/candy_types.h"

#define candy_lengthof(array) (sizeof(array) / sizeof(array[0]))

candy_hash_t candy_hash(char str[]);

static inline bool is_capital(char ch) {
  return (ch >= 'A') && (ch <= 'Z');
}

static inline bool is_lower(char ch) {
  return (ch >= 'a') && (ch <= 'z');
}

static inline bool is_alpha(char ch) {
  return is_capital(ch) || is_lower(ch);
}

static inline bool is_oct(char ch) {
  return (ch >= '0') && (ch <= '7');
}

static inline bool is_dec(char ch) {
  return (ch >= '0') && (ch <= '9');
}

static inline bool is_hex(char ch) {
  return is_dec(ch) || ((ch >= 'a') && (ch <= 'f')) || ((ch >= 'A') && (ch <= 'F'));
}

#ifdef __cplusplus
}
#endif /* __cplusplus */
#endif /* CANDY_SRC_LIB_H */
