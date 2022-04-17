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

#define is_capital(ch)            ((ch) >= 'A' && (ch) <= 'Z')
#define is_lower(ch)              ((ch) >= 'a' && (ch) <= 'z')
#define is_alpha(ch)              (is_capital(ch) || is_lower(ch))
#define is_hex(ch)                (((ch) >= '0' && (ch) <= '9') || ((ch) >= 'a' && (ch) <= 'f') || ((ch) >= 'A' && (ch) <= 'F'))
#define is_dec(ch)                ((ch) >= '0' && (ch) <= '9')
#define is_oct(ch)                ((ch) >= '0' && (ch) <= '7')

typedef enum candy_token_type{
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

static char *_get_oct(char *dst, char *code){
  /* save oct */
  char temp[] = {*code, '\0', '\0', '\0'};
  /* skip *code */
  code++;
  for (unsigned i = 1; i < 3; i++){
    if (is_oct(*code))
      temp[i] = *code++;
    else
      break;
  }
  /* string to oct */
  *dst = strtol(temp, NULL, 8);
  return code;
}

static char *_get_hex(char *dst, char *code){
  /* check hex char */
  candy_assert(is_hex(*code) && is_hex(*(code + 1)));
  /* save hex */
  char temp[] = {*code, *(code + 1), '\0'};
  code += 2;
  /* string to hex */
  *dst = strtol(temp, NULL, 16);
  return code;
}

static char *_get_escape(char *dst, char *code){
  /* is octal */
  if (is_oct(*code)){
    /* string to octal */
    code = _get_oct(dst, code);
  }
  /* is hexadecimal */
  else if (*code == 'x'){
    /* skip x */
    code++;
    /* string to hexadecimal */
    code = _get_hex(dst, code);
  }
  else{
    switch (*code){
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
    code++;
  }
  return code;
}

/**
  * @brief  gets token of type string, like "hello world\n" 'A\tB\tC' "\x41\x42\x43" '\041\042\043'
  * @param  dst token buffer
  * @param  code source code, header contains quotes ' or "
  * @param  size size of string
  * @retval code's offset after token
  */
static char *_get_string(char *dst, char *code, uint32_t *size){
  char *temp = code;
  /* check escape */
  while (*(temp - 1) == '\\' || temp == code){
    /* find next ' or " */
    temp = strchr(temp + 1, *code);
    /* if not found */
    candy_assert(temp != NULL, string not closed);
  }
  /* skip first ' or " */
  code++;
  /* store characters up to the last ' or " */
  while (code < temp){
    /* is escape */
    if (*code == '\\'){
      /* skip \ */
      code++;
      code = _get_escape(dst++, code);
      *size += 1;
    }
    else{
      *dst++ = *code++;
      *size += 1;
    }
  }
  *dst = '\0';
  /* skip last ' or " */
  code++;
  return code;
}

int candy_parser_gen_ast(candy_object_t root, char * const code){
  candy_assert(root != NULL);
  candy_assert(code != NULL);
  uint32_t line = 1;
  char buffer[CANDY_PARSER_BUFFER_SIZE] = {0};
  char *src = code;
  char *dst = buffer;
  /* while until *src == '\0' */
  while (*src){
    /* ident or keyword */
    if (is_alpha(*src) || *src == '_'){
      candy_token_type_t type = CANDY_TOKEN_IDENT;
      /* save alpha, or '_' */
      *dst++ = *src++;
      /* save alpha, number, or '_' */
      while (is_dec(*src) || is_alpha(*src) || *src == '_')
        *dst++ = *src++;
      /* last char padding '\0' */
      *dst = '\0';
      /* check keyword */
      for (unsigned i = 0; i < candy_lengthof(candy_keyword); i++){
        if (strcmp(buffer, candy_keyword[i]) == 0){
          type = (candy_token_type_t)i;
          break;
        }
      }
      printf("%s\t\t%s\tline %d\n", (type == CANDY_TOKEN_IDENT) ? "ident" : "keyword", buffer, line);
      /* reset */
      dst = buffer;
    }
    else if (is_dec(*src)){
      uint8_t dot = 0;
      while (is_dec(*src)){
        *dst++ = *src++;
        if (*src == '.'){
          dot++;
          *dst++ = *src++;
        }
      }
      candy_assert(dot == 1);
      *dst = '\0';
      printf("[number]%s, line %d\n", buffer, line);
      dst = buffer;
    }
    /* if char start with ' or " */
    else if (*src == '\'' || *src == '\"'){
      const char comment[] = {*src, *src, '\0'};
      /* if is comment */
      if (strncmp(src, comment, 2) == 0){
        src += 3;
        char *temp = strstr(src, comment);
        candy_assert(temp != NULL);
        printf("comment\t%s\tline %d\n", comment, line);
        while (src < temp){
          if (*src == '\n')
            line++;
          src++;
        }
        src += 3;
      }
      /* else is const string */
      else{
        uint32_t size = 0;
        src = _get_string(buffer, src, &size);
        printf("const\t\t%s\tline %d\tsize %d\n", buffer, line, size);
      }
    }
    /* line count */
    else if (*src == '\n'){
      line++;
      src++;
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
    else if (*src == '#'){
      char *temp = strchr(src + 1, '\n');
      candy_assert(temp != NULL);
      printf("comment\t#\tline %d\n", line);
      src = temp;
    }
    /* skip else, such as '\r', '\t', ' ', et.al. */
    else{
      src++;
    }
  }
  return 1;
}
