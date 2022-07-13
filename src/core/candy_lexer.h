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
#ifndef CANDY_SRC_CORE_PARSER_H
#define CANDY_SRC_CORE_PARSER_H
#ifdef __cplusplus
extern "C"{
#endif /* __cplusplus */

#include "src/common/candy_types.h"

enum {
  CANDY_TK_MIN = '\x80',
  CANDY_TK_ERROR = '\xFF',
  CANDY_TK_NONE = '\x00',
  CANDY_TK_EOF,
  CANDY_TK_EOS,
  CANDY_TK_KW_FALSE,
  CANDY_TK_KW_TRUE,
  CANDY_TK_KW_AND,
  CANDY_TK_KW_OR,
  CANDY_TK_KW_NOT,
  CANDY_TK_KW_IF,
  CANDY_TK_KW_ELIF,
  CANDY_TK_KW_ELSE,
  CANDY_TK_KW_WHILE,
  CANDY_TK_KW_FOR,
  CANDY_TK_KW_BREAK,
  CANDY_TK_KW_CONTINUE,
  CANDY_TK_IDENT,
  CANDY_TK_CST_STRING,
  CANDY_TK_CST_INTEGER,
  CANDY_TK_CST_FLOAT,
  CANDY_TK_CST_BOOLEAN,
  CANDY_TK_OPE_ASSIGN   = '=',                /* 0x3D =  */
  CANDY_TK_OPE_ADD      = '+',                /* 0x2B +  */
  CANDY_TK_OPE_SUB      = '-',                /* 0x2D -  */
  CANDY_TK_OPE_MUL      = '*',                /* 0x2A *  */
  CANDY_TK_OPE_DIV      = '/',                /* 0x2F /  */
  CANDY_TK_OPE_MOD      = '%',                /* 0x25 %  */
  CANDY_TK_OPE_GREATER  = '>',                /* 0x3E >  */
  CANDY_TK_OPE_LESS     = '<',                /* 0x3C <  */
  CANDY_TK_OPE_EQUAL    = '=' + CANDY_TK_MIN, /* 0xBD == */
  CANDY_TK_OPE_ADDASS   = '+' + CANDY_TK_MIN, /* 0xAB += */
  CANDY_TK_OPE_SUBASS   = '-' + CANDY_TK_MIN, /* 0xAD -= */
  CANDY_TK_OPE_MULASS   = '*' + CANDY_TK_MIN, /* 0xAA *= */
  CANDY_TK_OPE_DIVASS   = '/' + CANDY_TK_MIN, /* 0xAF /= */
  CANDY_TK_OPE_MODASS   = '%' + CANDY_TK_MIN, /* 0xB5 %= */
  CANDY_TK_OPE_GEQUAL   = '>' + CANDY_TK_MIN, /* 0xBE >= */
  CANDY_TK_OPE_LEQUAL   = '<' + CANDY_TK_MIN, /* 0xBC <= */
  CANDY_TK_OPE_NEQUAL   = '!' + CANDY_TK_MIN, /* 0xA1 != */
  /* todo: ** // */
  CANDY_TK_DEL_LPAREN  = '(',
  CANDY_TK_DEL_RPAREN  = ')',
  CANDY_TK_MAX,
#if CANDY_TK_MAX > '\x7F'
#error "too many tokens"
#endif /* CANDY_TK_MAX */
};

typedef union candy_meta{
  size_t value;
  candy_integer_t i;
  candy_float_t f;
  candy_hash_t hash;
  candy_wrap_t wrap;
} candy_meta_t;

struct candy_lexer;

typedef struct candy_lexer * candy_lexer_t;

candy_lexer_t candy_lexer_create(const char code[]);
int candy_lexer_delete(candy_lexer_t *lex);

int8_t candy_lexer_curr(candy_lexer_t lex, candy_meta_t *meta);
int8_t candy_lexer_lookahead(candy_lexer_t lex);

#ifdef __cplusplus
}
#endif /* __cplusplus */
#endif /* CANDY_SRC_CORE_PARSER_H */
