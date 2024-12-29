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
#ifndef CANDY_CORE_LEXER_H
#define CANDY_CORE_LEXER_H
#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#include "core/candy_buffer.h"
#include "core/candy_token.h"
#include "core/candy_priv.h"

typedef union candy_meta candy_meta_t;

union candy_meta {
  candy_integer_t i;
  candy_float_t f;
  candy_array_t *s;
};

struct candy_lexer {
  candy_buffer_t buff;
  struct {
    size_t line;
    size_t column;
  } dbg;
  struct {
    candy_tokens_t token;
    candy_meta_t meta;
  } lookahead;
  candy_excep_t *ctx;
  candy_gc_t *gc;
};

typedef struct candy_lexer candy_lexer_t;

int candy_lexer_init(candy_lexer_t *self, candy_gc_t *gc, candy_excep_t *ctx, candy_reader_t reader, void *arg);
int candy_lexer_deinit(candy_lexer_t *self);

candy_tokens_t candy_lexer_lookahead(candy_lexer_t *self);
const candy_meta_t *candy_lexer_next(candy_lexer_t *self);

#ifdef __cplusplus
}
#endif /* __cplusplus */
#endif /* CANDY_CORE_LEXER_H */
