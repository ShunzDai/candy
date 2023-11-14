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
#ifndef CANDY_SRC_PARSER_H
#define CANDY_SRC_PARSER_H
#ifdef __cplusplus
extern "C"{
#endif /* __cplusplus */

#include "src/candy_wrap.h"
#include "src/candy_types.h"

#define binary_ope(_l, _r) ((_l) << 8 | (_r))

typedef enum candy_tokens {
  TK_EOS,
  TK_PERCENT =                  '%', /* %  */
  TK_AMPER   =                  '&', /* &  */
  TK_LPAREN  =                  '(', /* (  */
  TK_RPAREN  =                  ')', /* )  */
  TK_ASTE    =                  '*', /* *  */
  TK_PLUS    =                  '+', /* +  */
  TK_COMMA   =                  ',', /* ,  */
  TK_MINUS   =                  '-', /* -  */
  TK_DOT     =                  '.', /* .  */
  TK_SLASH   =                  '/', /* /  */
  TK_COLON   =                  ':', /* :  */
  TK_LESS    =                  '<', /* <  */
  TK_ASSIGN  =                  '=', /* =  */
  TK_GREATER =                  '>', /* >  */
  TK_LSQUARE =                  '[', /* [  */
  TK_RSQUARE =                  ']', /* ]  */
  TK_CARET   =                  '^', /* ^  */
  TK_LBRACE  =                  '{', /* {  */
  TK_VERT    =                  '|', /* |  */
  TK_RBRACE  =                  '}', /* }  */
  TK_TILDE   =                  '~', /* ~  */
  TK_NEQUAL  = binary_ope('!', '='), /* != */
  TK_MODASS  = binary_ope('%', '='), /* %= */
  TK_EXP     = binary_ope('*', '*'), /* ** */
  TK_MULASS  = binary_ope('*', '='), /* *= */
  TK_ADDASS  = binary_ope('+', '='), /* += */
  TK_SUBASS  = binary_ope('-', '='), /* -= */
  TK_FLRDIV  = binary_ope('/', '/'), /* // */
  TK_DIVASS  = binary_ope('/', '='), /* /= */
  TK_LSHIFT  = binary_ope('<', '<'), /* << */
  TK_LEQUAL  = binary_ope('<', '='), /* <= */
  TK_EQUAL   = binary_ope('=', '='), /* == */
  TK_GEQUAL  = binary_ope('>', '='), /* >= */
  TK_RSHIFT  = binary_ope('>', '>'), /* >> */
  TK_INTEGER,
  TK_FLOAT,
  TK_STRING,
  TK_IDENT,
  TK_VARARG,
  #define CANDY_KW_ENUM
  #include "src/candy_keyword.list"
} candy_tokens_t;

/** @ref doc/io_memory_model.drawio.png */
struct candy_lexer {
  candy_io_t *io;
  struct {
    uint16_t line;
    uint16_t column;
  } dbg;
  struct {
    candy_tokens_t token;
    candy_wrap_t wrap;
  } lookahead;
  int w;
  int r;
  candy_reader_t reader;
  void *ud;
};

typedef struct candy_lexer candy_lexer_t;

int candy_lexer_init(candy_lexer_t *self, candy_io_t *io, candy_reader_t reader, void *ud);
int candy_lexer_deinit(candy_lexer_t *self);

candy_tokens_t candy_lexer_lookahead(candy_lexer_t *self);
const candy_wrap_t *candy_lexer_next(candy_lexer_t *self);

#ifdef __cplusplus
}
#endif /* __cplusplus */
#endif /* CANDY_SRC_PARSER_H */
