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
#include <stdbool.h>
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

#define CANDY_OBJECT_PARAM                  ((char *)"__param__")
#define CANDY_OBJECT_METHOD                 ((char *)"__method__")
#define CANDY_OBJECT_BLOCK                  ((char *)"__block__")
#define CANDY_OBJECT_AST                    ((char *)"__ast__")

#define candy_lengthof(array) (sizeof(array) / sizeof(array[0]))

#define candy_assert(condition, ...) ((condition) ? ((void)0U) : candy_platform_assert_error(__FILE__, __LINE__, __FUNCTION__, #condition " " __VA_ARGS__))

typedef struct candy_view{
  const uint16_t size;
  const uint8_t data[];
} * candy_view_t;

typedef struct candy_node * candy_node_t;

typedef struct candy_queue * candy_queue_t;

typedef struct candy_wrap * candy_wrap_t;

typedef struct candy_object * candy_object_t;

typedef uint32_t        candy_hash_t;
typedef candy_view_t    candy_string_t;
typedef int64_t         candy_integer_t;
typedef double          candy_float_t;
typedef uint8_t         candy_boolean_t;
typedef int (*candy_method_t)(candy_object_t);

typedef int (*candy_destroy_t)(candy_node_t *node);

typedef struct candy_register{
  const char *name;
  candy_method_t method;
} candy_register_t;

void candy_platform_assert_error(const char *file, int line, const char *func, char *format, ...);

#ifdef __cplusplus
}
#endif /* __cplusplus */
#endif /* CANDY_SRC_COMMON_TYPES_H */
