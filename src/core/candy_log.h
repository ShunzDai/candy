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
#ifndef CANDY_CORE_LOG_H
#define CANDY_CORE_LOG_H
#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#include "candy_config.h"

typedef enum candy_log_level {
  CANDY_LOG_LEVEL_N,
  CANDY_LOG_LEVEL_E,
  CANDY_LOG_LEVEL_W,
  CANDY_LOG_LEVEL_I,
  CANDY_LOG_LEVEL_D,
  CANDY_LOG_LEVEL_V,
} candy_log_level_t;

#define CANDY_LOG_COLOR_BLACK   "30"
#define CANDY_LOG_COLOR_RED     "31"
#define CANDY_LOG_COLOR_GREEN   "32"
#define CANDY_LOG_COLOR_BROWN   "33"
#define CANDY_LOG_COLOR_BLUE    "34"
#define CANDY_LOG_COLOR_PURPLE  "35"
#define CANDY_LOG_COLOR_CYAN    "36"
#define CANDY_LOG_COLOR_WHITE   "37"
#if CANDY_CONFIG_LOG_COLOR
#define CANDY_LOG_COLOR(COLOR)  "\033[0;" COLOR "m"
#define CANDY_LOG_BOLD(COLOR)   "\033[1;" COLOR "m"
#define CANDY_LOG_RESET_COLOR   "\033[0m"
#else /* CANDY_CONFIG_LOG_COLOR */
#define CANDY_LOG_COLOR(COLOR)
#define CANDY_LOG_BOLD(COLOR)
#define CANDY_LOG_RESET_COLOR
#endif /* CANDY_CONFIG_LOG_COLOR */

#define CANDY_LOG_COLOR_E       CANDY_LOG_COLOR(CANDY_LOG_COLOR_RED)
#define CANDY_LOG_COLOR_W       CANDY_LOG_COLOR(CANDY_LOG_COLOR_BROWN)
#define CANDY_LOG_COLOR_I       CANDY_LOG_COLOR(CANDY_LOG_COLOR_GREEN)
#define CANDY_LOG_COLOR_D       CANDY_LOG_COLOR(CANDY_LOG_COLOR_CYAN)
#define CANDY_LOG_COLOR_V       CANDY_LOG_COLOR(CANDY_LOG_COLOR_WHITE)

#define candy_log(_level, _tag, _fmt, ...) do { \
  if (CANDY_CONFIG_LOG_LEVEL >= CANDY_LOG_LEVEL_##_level) { \
    candy_log_impl(CANDY_LOG_COLOR_##_level #_level "(%s) " _fmt CANDY_LOG_RESET_COLOR "\n", _tag, __VA_ARGS__); \
  } \
} while(0)

#define candy_loge(_tag, ...) candy_log(E, _tag, __VA_ARGS__)
#define candy_logw(_tag, ...) candy_log(W, _tag, __VA_ARGS__)
#define candy_logi(_tag, ...) candy_log(I, _tag, __VA_ARGS__)
#define candy_logd(_tag, ...) candy_log(D, _tag, __VA_ARGS__)
#define candy_logv(_tag, ...) candy_log(V, _tag, __VA_ARGS__)

#ifdef __cplusplus
}
#endif /* __cplusplus */
#endif /* CANDY_CORE_LOG_H */
