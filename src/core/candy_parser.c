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

int candy_parser_gen_ast(candy_object_t root, char *code){
  candy_assert(root != NULL);
  candy_assert(code != NULL);
  uint32_t line = 1;
  char buffer[CANDY_PARSER_BUFFER_SIZE] = {0};
  char *ch = code;
  char *b = buffer;
  /* while until *ch == '\0' */
  while (*ch){
    /* ident or keyword */
    if (is_alpha(*ch) || *ch == '_'){
      candy_token_type_t type = CANDY_TOKEN_IDENT;
      /* save alpha, or '_' */
      *b++ = *ch++;
      /* save alpha, number, or '_' */
      while (is_dec(*ch) || is_alpha(*ch) || *ch == '_')
        *b++ = *ch++;
      /* last char padding '\0' */
      *b = '\0';
      /* check keyword */
      for (unsigned i = 0; i < candy_lengthof(candy_keyword); i++){
        if (strcmp(buffer, candy_keyword[i]) == 0){
          type = (candy_token_type_t)i;
          break;
        }
      }
      printf("[%s]%s, line %d\n", (type == CANDY_TOKEN_IDENT) ? "ident" : "keyword", buffer, line);
      /* reset */
      b = buffer;
    }
    else if (is_dec(*ch)){
      uint8_t dot = 0;
      while (is_dec(*ch)){
        *b++ = *ch++;
        if (*ch == '.'){
          dot++;
          *b++ = *ch++;
        }
      }
      candy_assert(dot == 1);
      *b = '\0';
      printf("[number]%s, line %d\n", buffer, line);
      b = buffer;
    }
    /* if char start with ' or " */
    else if (*ch == '\'' || *ch == '\"'){
      const char * const comment = (*ch == '\'') ? "\'\'\'" : "\"\"\"";
      /* if is comment */
      if (strncmp(ch + 1, comment, 2) == 0){
        ch += 3;
        char *temp = strstr(ch, comment);
        candy_assert(temp != NULL);
        printf("[comment]%s, line %d\n", comment, line);
        while (ch < temp){
          if (*ch == '\n')
            line++;
          ch++;
        }
        ch += 3;
      }
      /* else is const string */
      else{
        /* find next ' or " */
        char *temp = strchr(ch + 1, *ch);
        /* if not found */
        candy_assert(temp != NULL);
        /* char check */
        while (*(temp - 1) == '\\'){
          /* find next ' or " */
          temp = strchr(temp + 1, *ch);
          /* if not found */
          candy_assert(temp != NULL);
        }
        /* skip ' or " */
        ch++;
        while (ch < temp){
          /* is escape */
          if (*ch == '\\'){
            ch++;
            /* is '\a' */
            if (*ch == 'a'){
              *b++ = '\a';
              ch++;
            }
            /* is '\b' */
            else if (*ch == 'b'){
              *b++ = '\b';
              ch++;
            }
            /* is '\t' */
            else if (*ch == 't'){
              *b++ = '\t';
              ch++;
            }
            /* is '\n' */
            else if (*ch == 'n'){
              *b++ = '\n';
              ch++;
            }
            /* is '\v' */
            else if (*ch == 'v'){
              *b++ = '\v';
              ch++;
            }
            /* is '\f' */
            else if (*ch == 'f'){
              *b++ = '\f';
              ch++;
            }
            /* is '\r' */
            else if (*ch == 'r'){
              *b++ = '\r';
              ch++;
            }
            /* is '\"' */
            else if (*ch == '\"'){
              *b++ = '\"';
              ch++;
            }
            /* is '\'' */
            else if (*ch == '\''){
              *b++ = '\'';
              ch++;
            }
            /* is '\\' */
            else if (*ch == '\\'){
              *b++ = '\\';
              ch++;
            }
            /* is hex */
            else if (*ch == 'x'){
              ch++;
              /* check hex char */
              candy_assert(is_hex(*ch) && is_hex(*(ch + 1)));
              /* save hex */
              char temp[3] = {*ch, *(ch + 1), '\0'};
              ch += 2;
              /* string to hex */
              *b++ = strtol(temp, NULL, 16);
            }
            /* is oct */
            else if (is_oct(*ch)){
              /* save oct */
              char temp[4] = {*ch};
              ch++;
              for (unsigned i = 1; i < 3; i++){
                if (is_oct(*ch))
                  temp[i] = *ch++;
                else
                  break;
              }
              /* string to oct */
              *b++ = strtol(temp, NULL, 8);
            }
            else{
              candy_assert(0);
            }
          }
          else{
            *b++ = *ch++;
          }
        }
        *b = '\0';
        ch++;
        printf("[const]%s, line %d\n", buffer, line);
        b = buffer;
      }
    }
    /* line count */
    else if (*ch == '\n'){
      line++;
      ch++;
    }
    else if (*ch == '('){
      printf("[delimiter]'(', line %d\n", line);
      ch++;
    }
    else if (*ch == ')'){
      printf("[delimiter]')', line %d\n", line);
      ch++;
    }
    else if (*ch == '='){
      printf("[delimiter]'=', line %d\n", line);
      ch++;
    }
    else if (*ch == ','){
      printf("[delimiter]',', line %d\n", line);
      ch++;
    }
    else if (*ch == '#'){
      char *temp = strchr(ch + 1, '\n');
      candy_assert(temp != NULL);
      printf("[comment]#, line %d\n", line);
      ch = temp;
    }
    /* skip else, such as '\r', '\t', ' ', et.al. */
    else{
      ch++;
    }
  }
  return 1;
}
