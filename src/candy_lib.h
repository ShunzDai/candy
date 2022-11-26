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

#include <stdint.h>

#define candy_lengthof(array) (sizeof(array) / sizeof(array[0]))

static inline uint32_t djb_hash(char str[]) {
  uint32_t hash = 5381;
  while (*str)
    hash += (hash << 5) + (*str++);
  return (hash & 0x7FFFFFFF);
}

static inline bool is_capital(char ch) {
  return (unsigned)(ch - 'A') <= (unsigned)('Z' - 'A');
}

static inline bool is_lower(char ch) {
  return (unsigned)(ch - 'a') <= (unsigned)('z' - 'a');
}

static inline bool is_alpha(char ch) {
  return is_capital(ch) || is_lower(ch);
}

static inline bool is_oct(char ch) {
  return (unsigned)(ch - '0') <= (unsigned)('7' - '0');
}

static inline bool is_dec(char ch) {
  return (unsigned)(ch - '0') <= (unsigned)('9' - '0');
}

static inline bool is_hex(char ch) {
  return is_dec(ch) || ((unsigned)(ch - 'a') <= (unsigned)('f' - 'a')) || ((unsigned)(ch - 'A') <= (unsigned)('F' - 'A'));
}

static inline uint8_t ch2hex(char ch) {
  return is_dec(ch) ? (ch - '0') : ((is_capital(ch) ? (ch - 'A') : (ch - 'a')) + 10);
}

#ifdef __cplusplus
}
#endif /* __cplusplus */
#endif /* CANDY_SRC_LIB_H */
