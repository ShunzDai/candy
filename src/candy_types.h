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

#include "candy_config.h"
#include <stdint.h>
#include <stdbool.h>

typedef struct candy_view {
  const uint16_t size;
  char data[];
} candy_view_t;

typedef int64_t candy_integer_t;
typedef double candy_float_t;
typedef uint8_t candy_boolean_t;

typedef enum candy_wraps {
  CANDY_NONE,
  CANDY_INTEGER,
  CANDY_FLOAT,
  CANDY_BOOLEAN,
  CANDY_STRING,
  CANDY_CFUNCTION,
  CANDY_OBJECT,
  CANDY_MAX,
} candy_wraps_t;

typedef union candy_wrap {
  struct {
    uint32_t type : 4;
    uint32_t      : 28;
  };
  struct {
    uint32_t      : 4;
    uint32_t size : 28;
    union {
      candy_integer_t sval[sizeof(long) / sizeof(candy_integer_t)];
      candy_integer_t *lval;
    };
  } i;
  struct {
    uint32_t      : 4;
    uint32_t size : 28;
    union {
      candy_float_t sval[sizeof(long) / sizeof(candy_float_t)];
      candy_float_t *lval;
    };
  } f;
  struct {
    uint32_t      : 4;
    uint32_t size : 28;
    union {
      candy_boolean_t sval[sizeof(long) / sizeof(candy_boolean_t)];
      candy_boolean_t *lval;
    };
  } b;
  struct {
    uint32_t      : 4;
    uint32_t size : 28;
    union {
      char sval[sizeof(long) / sizeof(char)];
      char *lval;
    };
  } s;
} candy_wrap_t;

typedef struct candy_io candy_io_t;

typedef struct candy_lexer candy_lexer_t;

typedef struct candy_vm candy_vm_t;

typedef struct candy_state candy_state_t;

typedef int (*candy_reader_t)(char *, int, void *);

/**
  * @brief c-function
  */
typedef int (*candy_cfunc_t)(candy_state_t *, void *);

typedef struct candy_regist {
  const char *name;
  candy_cfunc_t cfunc;
} candy_regist_t;

#ifdef __cplusplus
}
#endif /* __cplusplus */
#endif /* CANDY_SRC_COMMON_TYPES_H */
