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
#include "candy_parser.h"
#include "src/struct/candy_object.h"
#include "candy_config.h"
#include <stdlib.h>

#define _is_capital(ch)           ((ch) >= 'A' && (ch) <= 'Z')
#define _is_lower(ch)             ((ch) >= 'a' && (ch) <= 'z')
#define _is_alpha(ch)             (_is_capital(ch) || _is_lower(ch))
#define _is_hex(ch)               (((ch) >= '0' && (ch) <= '9') || ((ch) >= 'a' && (ch) <= 'f') || ((ch) >= 'A' && (ch) <= 'F'))
#define _is_dec(ch)               ((ch) >= '0' && (ch) <= '9')
#define _is_oct(ch)               ((ch) >= '0' && (ch) <= '7')

typedef enum candy_token_type{
  CANDY_TOKEN_NULL,
  CANDY_TOKEN_KEYWORD_FALSE,
  CANDY_TOKEN_KEYWORD_TRUE,
  CANDY_TOKEN_KEYWORD_AND,
  CANDY_TOKEN_KEYWORD_OR,
  CANDY_TOKEN_KEYWORD_NOT,
  CANDY_TOKEN_KEYWORD_IF,
  CANDY_TOKEN_KEYWORD_ELSE,
  CANDY_TOKEN_KEYWORD_ELIF,
  CANDY_TOKEN_KEYWORD_WHILE,
  CANDY_TOKEN_KEYWORD_FOR,
  CANDY_TOKEN_KEYWORD_BREAK,
  CANDY_TOKEN_KEYWORD_CONTINUE,
  CANDY_TOKEN_IDENT,
  CANDY_TOKEN_CONST_STRING,
  CANDY_TOKEN_CONST_INTEGER,
  CANDY_TOKEN_CONST_FLOAT,
  CANDY_TOKEN_CONST_BOOLEAN,
  CANDY_TOKEN_OPERATOR,
  CANDY_TOKEN_DELIMITER_LPAREN,
  CANDY_TOKEN_DELIMITER_RPAREN,
  CANDY_TOKEN_MAX,
} candy_token_type_t;

static const char *const candy_keyword[] = {
  "False",
  "True",
  "and",
  "or",
  "not",
  "if",
  "else",
  "elif",
  "while",
  "for",
  "break",
  "continue",
};

static char *_get_comment(char *code, uint32_t *line){
  char *src = code;
  char *temp = NULL;
  const char comment[] = {*code, *code, *code, '\0'};
  switch (*src){
    case '#':
      temp = strchr(src + 1, '\n');
      candy_assert(temp != NULL, code have to leaving an extra blank line at the end);
      *line += 1;
      src = temp + 1;
      break;
    /* " or ' */
    default:
      src += 3;
      temp = strstr(src, comment);
      candy_assert(temp != NULL, comment not closed);
      while (src < temp){
        if (*src == '\n')
          *line += 1;
        src++;
      }
      src += 3;
      break;
  }
  return src;
}

static char *_get_ident_or_keyword(char *code, char *buff, candy_token_type_t *type){
  char *src = code;
  char *dst = buff;
  /* save alpha, or '_' */
  *dst++ = *src++;
  /* save number， alpha, or '_' */
  while (_is_dec(*src) || _is_alpha(*src) || *src == '_')
    *dst++ = *src++;
  /* last char padding '\0' */
  *dst = '\0';
  /* check keyword */
  *type = CANDY_TOKEN_IDENT;
  for (unsigned i = 0; i < candy_lengthof(candy_keyword); i++){
    if (strcmp(buff, candy_keyword[i]) == 0){
      *type = (candy_token_type_t)i;
      break;
    }
  }
  return src;
}

static char *_get_hex(char *code, char *buff){
  char *src = code;
  char *dst = buff;
  /* check hex char */
  candy_assert(_is_hex(*src) && _is_hex(*(src + 1)));
  /* save hex */
  char temp[] = {*src, *(src + 1), '\0'};
  src += 2;
  /* string to hex */
  *dst++ = strtol(temp, NULL, 16);
  *dst = '\0';
  return src;
}

static char *_get_dec(char *code, char *buff, candy_token_type_t *type){
  char *src = code;
  char *dst = buff;
  uint8_t dot = 0;
  while (_is_dec(*src)){
    *dst++ = *src++;
    if (*src == '.'){
      *dst++ = *src++;
      dot++;
    }
  }
  candy_assert(dot <= 1);
  *type = dot ? CANDY_TOKEN_CONST_FLOAT : CANDY_TOKEN_CONST_INTEGER;
  *dst = '\0';
  return src;
}

static char *_get_oct(char *code, char *buff){
  char *src = code;
  char *dst = buff;
  /* save oct */
  char temp[] = {*src, '\0', '\0', '\0'};
  /* skip *code */
  src++;
  for (unsigned i = 1; i < 3; i++){
    if (_is_oct(*src))
      temp[i] = *src++;
    else
      break;
  }
  /* string to oct */
  *dst++ = strtol(temp, NULL, 8);
  *dst = '\0';
  return src;
}

static char *_get_escape(char *code, char *buff){
  char *src = code;
  char *dst = buff;
  /* is hexadecimal */
  if (*src == 'x'){
    /* skip x */
    src++;
    /* string to hexadecimal */
    src = _get_hex(src, dst);
  }
  /* is octal */
  else if (_is_oct(*src)){
    /* string to octal */
    src = _get_oct(src, dst);
  }
  else{
    switch (*src){
      case 'a':   *dst = '\a';   break;
      case 'b':   *dst = '\b';   break;
      case 't':   *dst = '\t';   break;
      case 'n':   *dst = '\n';   break;
      case 'v':   *dst = '\v';   break;
      case 'f':   *dst = '\f';   break;
      case 'r':   *dst = '\r';   break;
      case '\"':  *dst = '\"';   break;
      case '\'':  *dst = '\'';   break;
      case '\\':  *dst = '\\';   break;
      default:    candy_assert(0);    break;
    }
    src++;
  }
  return src;
}

/**
  * @brief  gets token of type string, like "hello world\n" 'A\tB\tC' "\x41\x42\x43" '\041\042\043'
  * @param  buff token buffer
  * @param  code source code, header contains quotes ' or "
  * @param  size size of string
  * @retval code's offset after token
  */
static char *_get_string(char *code, char *buff, uint32_t *size){
  char *src = code;
  char *dst = buff;
  char *temp = code;
  /* check escape */
  while (*(temp - 1) == '\\' || temp == src){
    /* find next ' or " */
    temp = strchr(temp + 1, *src);
    /* if not found */
    candy_assert(temp != NULL, string not closed);
  }
  /* skip first ' or " */
  src++;
  /* store characters up to the last ' or " */
  while (src < temp){
    /* is escape */
    if (*src == '\\'){
      /* skip \ */
      src++;
      src = _get_escape(src, dst++);
    }
    else{
      *dst++ = *src++;
    }
    *size += 1;
  }
  *dst = '\0';
  /* skip last ' or " */
  src++;
  return src;
}

int candy_parser_gen_ast(candy_object_t root, char * const code){
  candy_assert(root != NULL);
  candy_assert(code != NULL);
  uint32_t line = 1;
  candy_token_type_t type = CANDY_TOKEN_NULL;
  char buffer[CANDY_PARSER_BUFFER_SIZE] = {0};
  char *src = code;
  /* while until *src == '\0' */
  while (*src){
    /* ident or keyword */
    if (_is_alpha(*src) || *src == '_'){
      src = _get_ident_or_keyword(src, buffer, &type);
      printf("%s\t\t%s\tline %d\n", (type == CANDY_TOKEN_IDENT) ? "ident" : "keyword", buffer, line);
    }
    else if (_is_dec(*src)){
      src = _get_dec(src, buffer, &type);
      printf("%s\t\t%s\tline %d\n", (type == CANDY_TOKEN_CONST_INTEGER) ? "integer" : "float", buffer, line);
    }
    /* if char start with ' or " */
    else if (*src == '\'' || *src == '\"'){
      /* is multiline comment */
      if (*(src + 1) == *src && *(src + 2) == *src){
        printf("comment\t%c%c%c\tline %d\n", *src, *src, *src, line);
        src = _get_comment(src, &line);
      }
      /* is const string */
      else{
        uint32_t size = 0;
        src = _get_string(src, buffer, &size);
        printf("string\t\t%s\tline %d\tsize %d\n", buffer, line, size);
      }
    }
    else if (*src == '#'){
      printf("comment\t#\tline %d\n", line);
      src = _get_comment(src, &line);
    }
    else if (*src == '('){
      printf("delimiter\t%c\tline %d\n", *src, line);
      src++;
    }
    else if (*src == ')'){
      printf("delimiter\t%c\tline %d\n", *src, line);
      src++;
    }
    else if (*src == '='){
      printf("delimiter\t%c\tline %d\n", *src, line);
      src++;
    }
    else if (*src == ','){
      printf("delimiter\t%c\tline %d\n", *src, line);
      src++;
    }
    else if (*src == '\n'){
      line++;
      src++;
    }
    /* skip else, such as '\r', '\t', ' ', et.al. */
    else{
      candy_assert(*src == '\r' || *src == '\t' || *src == ' ', invalid character);
      src++;
    }
  }
  return 1;
}
