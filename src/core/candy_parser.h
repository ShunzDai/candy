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

typedef enum candy_token_type{
  CANDY_TOKEN_NULL,
  CANDY_TOKEN_ERROR,
  CANDY_TOKEN_KEYWORD_FALSE,
  CANDY_TOKEN_KEYWORD_TRUE,
  CANDY_TOKEN_KEYWORD_AND,
  CANDY_TOKEN_KEYWORD_OR,
  CANDY_TOKEN_KEYWORD_NOT,
  CANDY_TOKEN_KEYWORD_IF,
  CANDY_TOKEN_KEYWORD_ELIF,
  CANDY_TOKEN_KEYWORD_ELSE,
  CANDY_TOKEN_KEYWORD_WHILE,
  CANDY_TOKEN_KEYWORD_FOR,
  CANDY_TOKEN_KEYWORD_BREAK,
  CANDY_TOKEN_KEYWORD_CONTINUE,
  CANDY_TOKEN_IDENT,
  CANDY_TOKEN_CONST_STRING,
  CANDY_TOKEN_CONST_INTEGER,
  CANDY_TOKEN_CONST_FLOAT,
  CANDY_TOKEN_CONST_BOOLEAN,
  CANDY_TOKEN_OPERATOR_ASSIGN,
  CANDY_TOKEN_OPERATOR_EQUAL,
  CANDY_TOKEN_OPERATOR_NOT_EQUAL,
  CANDY_TOKEN_OPERATOR_LESS,
  CANDY_TOKEN_OPERATOR_LESS_EQUAL,
  CANDY_TOKEN_OPERATOR_GREATER,
  CANDY_TOKEN_OPERATOR_GREATER_EQUAL,
  CANDY_TOKEN_OPERATOR_PLUS,
  CANDY_TOKEN_OPERATOR_MINUS,
  CANDY_TOKEN_OPERATOR_MUL,
  CANDY_TOKEN_OPERATOR_DIV,
  CANDY_TOKEN_OPERATOR_MOD,
  CANDY_TOKEN_DELIMITER_LPAREN,
  CANDY_TOKEN_DELIMITER_RPAREN,
  CANDY_TOKEN_MAX,
} candy_token_type_t;

struct candy_lexer;

typedef struct candy_lexer * candy_lexer_t;

candy_lexer_t candy_lexer_create(char * code);
candy_lexer_t candy_lexer_delete(candy_lexer_t lex);

candy_token_type_t candy_lexer_get_token(candy_lexer_t lex, candy_pack_t *pack);

#ifdef __cplusplus
}
#endif /* __cplusplus */
#endif /* CANDY_SRC_CORE_PARSER_H */
