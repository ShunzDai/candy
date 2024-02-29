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
#ifndef CANDY_SRC_LIB_H
#define CANDY_SRC_LIB_H
#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#include "src/candy_types.h"
#include <ctype.h>

#define candy_lengthof(array) ((int)(sizeof(array) / sizeof(array[0])))

long strntol(const char nptr[], size_t size, char *endptr[], int base);
double strntod(const char nptr[], size_t size, char *endptr[]);

static inline uint32_t djb_hash(char str[], int size) {
  uint32_t hash = 5381;
  for (int idx = 0; idx < size; ++idx)
    hash += (hash << 5) + str[idx];
  return hash & 0x7FFFFFFF;
}

static inline bool is_power2(size_t n) {
  return (n & (n - 1)) == 0;
}

static inline size_t next_power2(size_t n) {
  --n;
  n |= n >> 1;
  n |= n >> 2;
  n |= n >> 4;
  n |= n >> 8;
  n |= n >> 16;
  n |= n >> 32;
  return ++n;
}

static inline size_t ceillog2(size_t n) {
  size_t result = 0;
  if (n & 0xFFFFFFFF00000000LU) { result += 32; n >>= 32; }
  if (n & 0x00000000FFFF0000LU) { result += 16; n >>= 16; }
  if (n & 0x000000000000FF00LU) { result +=  8; n >>=  8; }
  if (n & 0x00000000000000F0LU) { result +=  4; n >>=  4; }
  if (n & 0x000000000000000CLU) { result +=  2; n >>=  2; }
  if (n & 0x0000000000000002LU) { result +=  1; n >>=  1; }
  return result;
}

static inline bool is_upper(char ch) {
  return isupper(ch);
}

static inline bool is_lower(char ch) {
  return islower(ch);
}

static inline bool is_alpha(char ch) {
  return isalpha(ch) || ch == '_';
}

static inline bool is_alnum(char ch) {
  return isalnum(ch) || ch == '_';
}

static inline bool is_bin(char ch) {
  return (unsigned)(ch - '0') <= (unsigned)('1' - '0');
}

static inline bool is_oct(char ch) {
  return (unsigned)(ch - '0') <= (unsigned)('7' - '0');
}

static inline bool is_dec(char ch) {
  return isdigit(ch);
}

static inline bool is_hex(char ch) {
  return isxdigit(ch);
}

static inline uint8_t chtonum(char ch) {
  return isdigit(ch) ? (ch - '0') : isalpha(ch) ? (toupper(ch) - 'A' + 10) : -1;
}

#ifdef __cplusplus
}
#endif /* __cplusplus */
#endif /* CANDY_SRC_LIB_H */
