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

#define candy_assert(condition) ((condition) ? ((void *)0U) : printf("assert %s line: %d", __FUNCTION__, __LINE__))

typedef enum candy_status{
  CANDY_OK                                  = 0x00U,
  CANDY_ERROR                               = 0x01U,
  CANDY_BUSY                                = 0x02U,
  CANDY_TIMEOUT                             = 0x03U,
}candy_status_t;

#ifdef __cplusplus
}
#endif /* __cplusplus */
#endif /* CANDY_SRC_COMMON_TYPES_H */
