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

#define dual_ope(_l, _r) ((uint8_t)(((_l) * (_r)) % 0xFF) | 0x80)

typedef enum candy_tokens {
  TK_NULL = '\x00',
  TK_EOS,
  TK_STRING,
  TK_INTEGER,
  TK_FLOAT,
  TK_IDENT,
  TK_VARARG,
  #define CANDY_KW_ENUM
  #include "src/candy_keyword.list"
  TK_LPAREN   =                '(', /* 0x28 (  */
  TK_RPAREN   =                ')', /* 0x29 )  */
  TK_COMMA    =                ',', /* 0x2C ,  */
  TK_DOT      =                '.', /* 0x2E .  */
  TK_COLON    =                ':', /* 0x3A :  */
  TK_LBRACE   =                '[', /* 0x5B [  */
  TK_RBRACE   =                ']', /* 0x5D ]  */
  TK_BITAND   =                '&', /* 0x26 &  */
  TK_BITOR    =                '|', /* 0x7C |  */
  TK_BITNOT   =                '~', /* 0x7E ~  */
  TK_BITXOR   =                '^', /* 0x5E ^  */
  TK_MOD      =                '%', /* 0x25 %  */
  TK_ADD      =                '+', /* 0x2B +  */
  TK_SUB      =                '-', /* 0x2D -  */
  TK_MUL      =                '*', /* 0x2A *  */
  TK_DIV      =                '/', /* 0x2F /  */
  TK_ASSIGN   =                '=', /* 0x3D =  */
  TK_GREATER  =                '>', /* 0x3E >  */
  TK_LESS     =                '<', /* 0x3C <  */
  TK_EXP      = dual_ope('*', '*'), /* 0xEA ** */
  TK_FLRDIV   = dual_ope('/', '/'), /* 0xA9 // */
  TK_MODASS   = dual_ope('%', '='), /* 0xD9 %= */
  TK_NEQUAL   = dual_ope('!', '='), /* 0xE4 != */
  TK_ADDASS   = dual_ope('+', '='), /* 0xC9 += */
  TK_SUBASS   = dual_ope('-', '='), /* 0xC3 -= */
  TK_MULASS   = dual_ope('*', '='), /* 0x8C *= */
  TK_DIVASS   = dual_ope('/', '='), /* 0xBE /= */
  TK_EQUAL    = dual_ope('=', '='), /* 0x97 == */
  TK_GEQUAL   = dual_ope('>', '='), /* 0xD4 >= */
  TK_LEQUAL   = dual_ope('<', '='), /* 0xDA <= */
  TK_RSHIFT   = dual_ope('>', '>'), /* 0x93 >> */
  TK_LSHIFT   = dual_ope('<', '<'), /* 0x9E << */
  TK_ERROR        = 0xFFU,
} candy_tokens_t;

typedef struct candy_token {
  candy_tokens_t token;
  candy_wrap_t wrap;
} candy_token_t;

/** @ref doc/io_memory_model.drawio.png */
struct candy_lexer {
  candy_io_t *io;
#ifdef CANDY_DEBUG_MODE
  struct {
    uint16_t line;
    uint16_t column;
  } dbg;
#endif /* CANDY_DEBUG_MODE */
  struct {
    uint8_t type;
  } indent;
  candy_token_t curr;
  candy_token_t lookahead;
  size_t w;
  size_t r;
  candy_reader_t reader;
  void *ud;
};

typedef struct candy_lexer candy_lexer_t;

int candy_lexer_init(candy_lexer_t *self, candy_io_t *io, candy_reader_t reader, void *ud);
int candy_lexer_deinit(candy_lexer_t *self);

candy_tokens_t candy_lexer_next(candy_lexer_t *self);
candy_tokens_t candy_lexer_lookahead(candy_lexer_t *self);

#ifdef __cplusplus
}
#endif /* __cplusplus */
#endif /* CANDY_SRC_PARSER_H */
