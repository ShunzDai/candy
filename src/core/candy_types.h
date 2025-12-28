/**
  * Copyright 2022-2025 ShunzDai
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
#ifndef CANDY_CORE_TYPES_H
#define CANDY_CORE_TYPES_H
#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#include "candy_config.h"
#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#define CANDY_MACRO_STR_IMPL(_val)   #_val
#define CANDY_MACRO_STR(_val)        CANDY_MACRO_STR_IMPL(_val)

#define CANDY_VERSION_MAJOR     0
#define CANDY_VERSION_MINOR     0
#define CANDY_VERSION_PATCH     1
#define CANDY_VERSION_STR       CANDY_MACRO_STR(CANDY_VERSION_MAJOR) "." CANDY_MACRO_STR(CANDY_VERSION_MINOR) "." CANDY_MACRO_STR(CANDY_VERSION_PATCH)
#define CANDY_VERSION_NUM       (CANDY_VERSION_MAJOR << 16 | CANDY_VERSION_MINOR << 8 | CANDY_VERSION_PATCH)

typedef enum candy_err {
  #define CANDY_ERR_ENUM
  #include "core/candy_error.list"
} candy_err_t;

typedef enum candy_types {
  #define CANDY_TYPE_ENUM
  #include "core/candy_type.list"
} candy_types_t;

typedef CANDY_CONFIG_INTEGER_TYPE candy_integer_t;
typedef CANDY_CONFIG_FLOAT_TYPE   candy_float_t;
typedef CANDY_CONFIG_BOOLEAN_TYPE candy_boolean_t;

typedef struct candy_state candy_state_t;

typedef int (*candy_reader_t)(void *buffer, size_t max_len, void *arg);

typedef void *(*candy_allocator_t)(void *prev, size_t prev_size, size_t next_size, void *arg);

/**
  * @brief c-type function
  */
typedef int (*candy_cfunc_t)(candy_state_t *self);

static inline const char *candy_err_str(candy_err_t err) {
  switch (err) {
    #define CANDY_ERR_STR
    #include "core/candy_error.list"
    default: return "unknown";
  }
}

static inline const char *candy_type_str(candy_types_t val) {
  return (const char *[]) {
    #define CANDY_TYPE_STR
    #include "core/candy_type.list"
  }[val];
}

#ifdef __cplusplus
}
#endif /* __cplusplus */
#endif /* CANDY_CORE_TYPES_H */
