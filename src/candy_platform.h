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
#ifndef CANDY_SRC_PLATFORM_PLATFORM_H
#define CANDY_SRC_PLATFORM_PLATFORM_H
#ifdef __cplusplus
extern "C"{
#endif /* __cplusplus */

#include <stdint.h>

#ifndef candy_assert
#define candy_assert(condition, ...) ((condition) ? ((void)0U) : candy_platform_assert(__FILE__, __LINE__, __FUNCTION__, #condition " " __VA_ARGS__))
#endif /* candy_assert */

void candy_platform_assert(const char *file, int line, const char *func, char *format, ...);

#ifdef __cplusplus
}
#endif /* __cplusplus */
#endif /* CANDY_SRC_PLATFORM_PLATFORM_H */
