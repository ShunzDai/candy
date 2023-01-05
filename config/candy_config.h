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
extern "C"{
#endif /* __cplusplus */

/**
  * @brief  the maximum number of bytes for the lexer to lookahead to.
  */
#define CANDY_LEXER_LOOKAHEAD_SIZE 3

/**
  * @brief  the maximum number of extra bytes that the user can write.
  */
#define CANDY_LEXER_EXTRA_SIZE 1

/**
  * @brief  smaller buffers mean less space utilization and more load times,
  *         which can be decided by the user depending on the usage scenario.
  */
#define CANDY_ATOMIC_BUFFER_SIZE 64

#if CANDY_ATOMIC_BUFFER_SIZE < CANDY_LEXER_EXTRA_SIZE + CANDY_LEXER_LOOKAHEAD_SIZE
#error "default buffer size is too small"
#endif /* CANDY_ATOMIC_BUFFER_SIZE */

#define CANDY_ATOMIC_STACK_SIZE 8

#ifdef __GNUC__
#define CANDY_NORETURN __attribute__((noreturn))
#else
#define CANDY_NORETURN
#endif

#ifdef __cplusplus
}
#endif /* __cplusplus */
#endif /* CANDY_CONFIG_H */
