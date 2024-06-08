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
#ifndef CANDY_SRC_TYPES_H
#define CANDY_SRC_TYPES_H
#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

typedef int64_t candy_integer_t;
typedef double candy_float_t;
typedef bool candy_boolean_t;

typedef struct candy_state candy_state_t;

typedef int (*candy_reader_t)(char buffer[], const size_t max_len, void *arg);

typedef void *(*candy_allocator_t)(void *ptr, size_t old_size, size_t new_size, void *arg);

/**
  * @brief c-type function
  */
typedef int (*candy_cfunc_t)(candy_state_t *self);

#ifdef __cplusplus
}
#endif /* __cplusplus */
#endif /* CANDY_SRC_TYPES_H */
