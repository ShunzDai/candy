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
#ifndef CANDY_SRC_PARSER_H
#define CANDY_SRC_PARSER_H
#ifdef __cplusplus
extern "C"{
#endif /* __cplusplus */

#include "src/candy_io.h"
#include "src/candy_wrap.h"
#include "src/candy_types.h"

#define tk_dual_ope(_l, _r) ((uint8_t)(((_l) * (_r)) % 0xFF) | 0x80)

typedef enum candy_tokens {
  CANDY_TK_NONE = '\x00',
  CANDY_TK_EOF,
  CANDY_TK_EOS,
  CANDY_TK_CST_STRING,
  CANDY_TK_CST_INTEGER,
  CANDY_TK_CST_FLOAT,
  CANDY_TK_IDENT,
  #define CANDY_KW_ENUM
  #include "src/candy_keyword.list"
  CANDY_TK_DEL_LPAREN   =                   '(', /* 0x28 (  */
  CANDY_TK_DEL_RPAREN   =                   ')', /* 0x29 )  */
  CANDY_TK_DEL_COMMA    =                   ',', /* 0x2C ,  */
  CANDY_TK_DEL_DOT      =                   '.', /* 0x2E .  */
  CANDY_TK_DEL_COLON    =                   ':', /* 0x3A :  */
  CANDY_TK_DEL_LBRACE   =                   '[', /* 0x5B [  */
  CANDY_TK_DEL_RBRACE   =                   ']', /* 0x5D ]  */
  CANDY_TK_OPE_BITAND   =                   '&', /* 0x26 &  */
  CANDY_TK_OPE_BITOR    =                   '|', /* 0x7C |  */
  CANDY_TK_OPE_BITNOT   =                   '~', /* 0x7E ~  */
  CANDY_TK_OPE_BITXOR   =                   '^', /* 0x5E ^  */
  CANDY_TK_OPE_MOD      =                   '%', /* 0x25 %  */
  CANDY_TK_OPE_ADD      =                   '+', /* 0x2B +  */
  CANDY_TK_OPE_SUB      =                   '-', /* 0x2D -  */
  CANDY_TK_OPE_MUL      =                   '*', /* 0x2A *  */
  CANDY_TK_OPE_DIV      =                   '/', /* 0x2F /  */
  CANDY_TK_OPE_ASSIGN   =                   '=', /* 0x3D =  */
  CANDY_TK_OPE_GREATER  =                   '>', /* 0x3E >  */
  CANDY_TK_OPE_LESS     =                   '<', /* 0x3C <  */
  CANDY_TK_OPE_EXP      = tk_dual_ope('*', '*'), /* 0xEA ** */
  CANDY_TK_OPE_FLRDIV   = tk_dual_ope('/', '/'), /* 0xA9 // */
  CANDY_TK_OPE_MODASS   = tk_dual_ope('%', '='), /* 0xD9 %= */
  CANDY_TK_OPE_NEQUAL   = tk_dual_ope('!', '='), /* 0xE4 != */
  CANDY_TK_OPE_ADDASS   = tk_dual_ope('+', '='), /* 0xC9 += */
  CANDY_TK_OPE_SUBASS   = tk_dual_ope('-', '='), /* 0xC3 -= */
  CANDY_TK_OPE_MULASS   = tk_dual_ope('*', '='), /* 0x8C *= */
  CANDY_TK_OPE_DIVASS   = tk_dual_ope('/', '='), /* 0xBE /= */
  CANDY_TK_OPE_EQUAL    = tk_dual_ope('=', '='), /* 0x97 == */
  CANDY_TK_OPE_GEQUAL   = tk_dual_ope('>', '='), /* 0xD4 >= */
  CANDY_TK_OPE_LEQUAL   = tk_dual_ope('<', '='), /* 0xDA <= */
  CANDY_TK_OPE_RSHIFT   = tk_dual_ope('>', '>'), /* 0x93 >> */
  CANDY_TK_OPE_LSHIFT   = tk_dual_ope('<', '<'), /* 0x9E << */
  CANDY_TK_ERROR        = 0xFFU,
} candy_tokens_t;

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
  struct {
    uint8_t token;
    candy_wrap_t wrap;
  } lookahead;
};

int candy_lexer_init(candy_lexer_t *self, candy_io_t *io);
int candy_lexer_deinit(candy_lexer_t *self);

candy_tokens_t candy_lexer_next(candy_lexer_t *self, candy_wrap_t *wrap);
candy_tokens_t candy_lexer_lookahead(candy_lexer_t *self);

#ifdef __cplusplus
}
#endif /* __cplusplus */
#endif /* CANDY_SRC_PARSER_H */
