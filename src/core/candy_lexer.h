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
#include "core/candy_priv.h"

#define gen_opr1(_byte1)                    (_byte1)
#define gen_opr2(_byte1, _byte2)            ((gen_opr1(_byte1)         << 8) | gen_opr1(_byte2))
#define gen_opr3(_byte1, _byte2, _byte3)    ((gen_opr2(_byte1, _byte2) << 8) | gen_opr1(_byte3))
#define gen_opr_select(_1, _2, _3, _n, ...) gen_opr##_n
#define gen_operator(...)                   gen_opr_select(__VA_ARGS__, 3, 2, 1)(__VA_ARGS__)

typedef enum candy_tokens {
  TK_EOS,
  TK_IDENT,
  TK_INTEGER,
  TK_FLOAT,
  TK_STRING,
  #define CANDY_OPR_ENUM
  #include "core/candy_operator.list"
  #define CANDY_KW_ENUM
  #include "core/candy_keyword.list"
} candy_tokens_t;

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
  candy_exce_t *ctx;
  candy_gc_t *gc;
};

typedef struct candy_lexer candy_lexer_t;

static inline const char *candy_token_str(candy_tokens_t token) {
  switch (token) {
    case TK_EOS:     return "EOS";
    case TK_IDENT:   return "ident";
    case TK_INTEGER: return "integer";
    case TK_FLOAT:   return "float";
    case TK_STRING:  return "string";
    #define CANDY_OPR_STR
    #include "core/candy_operator.list"
    #define CANDY_KW_STR
    #include "core/candy_keyword.list"
    default:         return "unknown";
  }
}

int candy_lexer_init(candy_lexer_t *self, candy_gc_t *gc, candy_exce_t *ctx, candy_reader_t reader, void *arg);
int candy_lexer_deinit(candy_lexer_t *self);

candy_tokens_t candy_lexer_lookahead(candy_lexer_t *self);
const candy_meta_t *candy_lexer_next(candy_lexer_t *self);

#ifdef __cplusplus
}
#endif /* __cplusplus */
#endif /* CANDY_CORE_LEXER_H */
