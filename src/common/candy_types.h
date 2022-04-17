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

#define CANDY_OBJECT_PARAM                  ((char *)"__param__")
#define CANDY_OBJECT_METHOD                 ((char *)"__method__")
#define CANDY_OBJECT_BLOCK                  ((char *)"__block__")
#define CANDY_OBJECT_AST                    ((char *)"__ast__")

#define candy_lengthof(array) (sizeof(array) / sizeof(array[0]))

#define candy_assert(condition, ...) ((condition) ? ((void)0U) : candy_platform_assert_error(__FILE__, __FUNCTION__, __LINE__, #condition " " #__VA_ARGS__))

typedef enum candy_types{
  CANDY_TYPES_NONE,
  CANDY_TYPES_STRING,
  CANDY_TYPES_INTEGER,
  CANDY_TYPES_FLOAT,
  CANDY_TYPES_BOOLEAN,
  CANDY_TYPES_METHOD,
  CANDY_TYPES_QUEUE,
  CANDY_TYPES_FLOOR,
} candy_types_t;

struct candy_state;

typedef struct candy_state * candy_state_t;

struct candy_node;

typedef struct candy_node * candy_node_t;

struct candy_queue;

typedef struct candy_queue * candy_queue_t;

struct candy_pack;

typedef struct candy_pack * candy_pack_t;

struct candy_bytecode;

typedef struct candy_bytecode * candy_bytecode_t;

struct candy_block;

typedef struct candy_block * candy_block_t;

typedef candy_queue_t candy_object_t;

typedef uint32_t        candy_hash_t;
typedef char *          candy_string_t;
typedef int64_t         candy_integer_t;
typedef float           candy_float_t;
typedef uint8_t         candy_boolean_t;
typedef int (*candy_method_t)(candy_object_t);

typedef struct candy_register{
  char *name;
  candy_method_t method;
} candy_register_t;

static const char *const candy_types[] = {
  "none",
  "string",
  "integer",
  "float",
  "bool",
  "method",
  "object",
  "unknown"
};

void candy_platform_assert_error(const char *file, const char *func, int line, char *condition);

#ifdef __cplusplus
}
#endif /* __cplusplus */
#endif /* CANDY_SRC_COMMON_TYPES_H */
