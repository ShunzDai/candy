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
#ifndef CANDY_CONFIG_H
#define CANDY_CONFIG_H
#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/**
  * @brief  larger mask means project can support more basic types and array size
  */
#define CANDY_MASK_TYPE uint16_t

/**
  * @brief  the logarithm base 2 of the number of basic types
  */
#define CANDY_BASIC_TYPE_SIZE 4

/**
  * @brief  smaller expand size mean less space utilization and more load times,
  *         which can be decided by the user depending on the usage scenario.
  */
#define CANDY_LEXER_EXPAND_SIZE 64

#define CANDY_DEFAULT_STACK_SIZE 8

#ifdef __cplusplus
}
#endif /* __cplusplus */
#endif /* CANDY_CONFIG_H */
