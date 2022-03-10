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
#ifndef CANDY_SRC_COMMON_TYPES_H
#define CANDY_SRC_COMMON_TYPES_H
#ifdef __cplusplus
extern "C"{
#endif /* __cplusplus */

#include <stdint.h>
#include <string.h>
#include <stdio.h>

#if defined(_MSC_FULL_VER)
#define CANDY_WEAK __weak
#elif defined(__ARMCC_VERSION)
#define CANDY_WEAK   __attribute__((weak))
#elif defined(__GNUC__)
#define CANDY_WEAK   __attribute__((weak))
#else
#define CANDY_WEAK
#endif

#define candy_assert(condition) ((condition) ? ((void)0U) : printf("Assertion failed: func %s, line %d\n", __FUNCTION__, __LINE__))

struct candy_pack;
typedef struct candy_pack * candy_pack_t;

struct candy_object;
typedef struct candy_object * candy_object_t;

typedef uint32_t        candy_hash_t;
typedef char *          candy_string_t;
typedef int64_t         candy_integer_t;
typedef float           candy_float_t;
typedef int (*candy_method_t)(candy_object_t obj);

typedef struct candy_register{
  char *name;
  candy_method_t method;
} candy_register_t;

#ifdef __cplusplus
}
#endif /* __cplusplus */
#endif /* CANDY_SRC_COMMON_TYPES_H */
