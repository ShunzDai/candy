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
#include "src/platform/candy_memory.h"
#include "src/struct/candy_pack.h"
#include "candy_config.h"
#include <stdlib.h>

#define _is_capital(ch)           ((ch) >= 'A' && (ch) <= 'Z')
#define _is_lower(ch)             ((ch) >= 'a' && (ch) <= 'z')
#define _is_alpha(ch)             (_is_capital(ch) || _is_lower(ch))
#define _is_hex(ch)               (((ch) >= '0' && (ch) <= '9') || ((ch) >= 'a' && (ch) <= 'f') || ((ch) >= 'A' && (ch) <= 'F'))
#define _is_dec(ch)               ((ch) >= '0' && (ch) <= '9')
#define _is_oct(ch)               ((ch) >= '0' && (ch) <= '7')



struct candy_lexer {
  char *code;
  char *curr;
  char *error;
  uint32_t line;
};

static const char *const _candy_keyword[] = {
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
  for (unsigned i = 0; i < candy_lengthof(_candy_keyword); i++){
    if (strcmp(buff, _candy_keyword[i]) == 0){
      *type = (candy_token_type_t)i;
      break;
    }
  }
  return src;
}

/**
  * @brief  gets hexadecimal escape character, like
  *         "30"
  * @param  lex lexer
  * @retval hexadecimal character
  */
static char _read_hex(candy_lexer_t lex){
  /* save hex */
  char hex[] = {lex->curr[0], lex->curr[1], '\0'};
  lex->curr += 2;
  /* string to hex */
  char *end = NULL;
  char res = strtol(hex, &end, 16);
  candy_assert(end == hex + 2, "hex not valid");
  return res;
}

/**
  * @brief  gets octal escape character, like
  *         "0"
  *         "01"
  *         "012"
  * @param  lex lexer
  * @retval octal character
  */
static char _read_oct(candy_lexer_t lex){
  /* save oct */
  char oct[] = {lex->curr[0], lex->curr[1], lex->curr[2], '\0'};
  lex->curr += 3;
  if (_is_oct(oct[1]) == false){
    oct[1] = '\0';
    lex->curr -= 2;
  }
  else if (_is_oct(oct[2]) == false){
    oct[2] = '\0';
    lex->curr -= 1;
  }
  /* string to oct */
  return strtol(oct, NULL, 8);
}

/**
  * @brief  gets token of type string, like
  *         "hello world\n"
  *         'A\tB\tC'
  *         "\x41\x42\x43"
  *         '\041\042\043'
  * @param  lex lexer
  * @param  buff store buffer, can not be NULL
  * @param  multiline is multiline string or not
  * @retval comment string length, not include '\0'
  */
static int _read_string(candy_lexer_t lex, char * buff, bool multiline){
  candy_assert(buff != NULL, "buffer is null");
  const char del = *lex->curr;
  char *tail = buff;
  /* skip first " or ' */
  lex->curr++;
  while (*lex->curr != del){
    switch (*lex->curr){
      case '\0':
        candy_assert(0, "unexpected end of string");
        return -1;
      case '\n':
        if (multiline == false){
          candy_assert(0, "while scanning string literal");
          return -1;
        }
        *tail++ = *lex->curr++;
        lex->line++;
        break;
      case '\\':
        lex->curr++;
        switch (*lex->curr){
          case         'a': lex->curr++; *tail++ =           '\a'; break;
          case         'b': lex->curr++; *tail++ =           '\b'; break;
          case         't': lex->curr++; *tail++ =           '\t'; break;
          case         'n': lex->curr++; *tail++ =           '\n'; break;
          case         'v': lex->curr++; *tail++ =           '\v'; break;
          case         'f': lex->curr++; *tail++ =           '\f'; break;
          case         'r': lex->curr++; *tail++ =           '\r'; break;
          case        '\\': lex->curr++; *tail++ =           '\\'; break;
          case        '\'': lex->curr++; *tail++ =           '\''; break;
          case         '"': lex->curr++; *tail++ =            '"'; break;
          case         'x': lex->curr++; *tail++ = _read_hex(lex); break;
          case '0' ... '7':              *tail++ = _read_oct(lex); break;
          /* TODO: support unicode */
          default:
            /* is not escape */
            //printf("unknown escape sequence: '\\%c'\n", *lex->curr);
            *tail++ = '\\';
            *tail++ = *lex->curr++;
            break;
        }
        break;
      /* normal character */
      default:
        *tail++ = *lex->curr++;
        break;
    }
  }
  *tail = '\0';
  /* skip last " or ' */
  lex->curr++;
  return tail - buff;
}

/**
  * @brief  get singleline comment, like
  *         # hello world
  * @param  lex lexer
  * @param  buff store buffer
  * @retval comment string length, not include '\0'
  */
static int _read_singleline_comment(candy_lexer_t lex, char *buff){
  char *head = lex->curr;
  char *tail = strchr(head, '\n');
  candy_assert(tail != NULL, "comment not closed");
  /* store */
  int len = tail - head - 1;
  if(buff != NULL){
    strncpy(buff, head + 1, len);
    buff[len] = '\0';
  }
  /* skip '\n' */
  lex->curr = tail + 1;
  lex->line++;
  return len;
}

/**
  * @brief  get multiline comment, like
  *         '''
  *         hello\nworld
  *         hello
  *         world
  *         '''
  * @param  lex lexer
  * @param  buff store buffer, can be NULL(will not store string to the buffer)
  * @retval comment string length
  */
static int _read_multiline_comment(candy_lexer_t lex, char * buff){
  const char del[] = {lex->curr[0], lex->curr[1], lex->curr[2], '\0'};
  int len = 0;
  if (buff){
    lex->curr += 2;
    len = _read_string(lex, buff, true);
    lex->curr += 2;
  }
  else{
    lex->curr += 3;
    char *tail = strstr(lex->curr, del);
    candy_assert(tail != NULL, "comment not closed");
    while (lex->curr != tail){
      if (*lex->curr++ == '\n')
        lex->line++;
      len++;
    }
    lex->curr += 3;
  }
  return len;
}

candy_lexer_t candy_lexer_create(char *code){
  candy_lexer_t lex = (candy_lexer_t)candy_malloc(sizeof(struct candy_lexer));
  lex->code = code;
  lex->curr = code;
  lex->line = 1;
  return lex;
}

candy_lexer_t candy_lexer_delete(candy_lexer_t lex){
  candy_free(lex);
  return NULL;
}

candy_token_type_t candy_lexer_get_token(candy_lexer_t lex, candy_pack_t *pack){
  candy_assert(lex != NULL);
  char buffer[CANDY_PARSER_BUFFER_SIZE] = {0};
  int len = 0;
  /* while until *src == '\0' */
  while (*lex->curr){
    switch (*lex->curr){
      case '\n': case '\r':
        lex->line++;
        lex->curr++;
        break;
      case ' ': case '\f': case '\t': case '\v':
        lex->curr++;
        break;
      case '=':
        if (*(lex->curr + 1) == '='){
          lex->curr += 2;
          return CANDY_TOKEN_OPERATOR_EQUAL;
        }
        lex->curr++;
        return CANDY_TOKEN_OPERATOR_ASSIGN;
      case '<':
        if (*(lex->curr + 1) == '='){
          lex->curr += 2;
          return CANDY_TOKEN_OPERATOR_LESS_EQUAL;
        }
        lex->curr++;
        return CANDY_TOKEN_OPERATOR_LESS;
      case '>':
        if (*(lex->curr + 1) == '='){
          lex->curr += 2;
          return CANDY_TOKEN_OPERATOR_GREATER_EQUAL;
        }
        lex->curr++;
        return CANDY_TOKEN_OPERATOR_GREATER;
      case '/':
        lex->curr++;
        return CANDY_TOKEN_OPERATOR_DIV;
      case '~':
        return -1;
      /* is singleline comment */
      case '#':
        printf("singleline comment\tline %d\n", lex->line);
        len = _read_singleline_comment(lex, buffer);
        printf("buff = '%s', len = %d\n", buffer, len);
        break;
      /* if char start with \' or \" */
      case '"': case '\'':
        /* is multiline comment */
        if (*(lex->curr + 1) == *lex->curr && *(lex->curr + 2) == *lex->curr){
          printf("multiline comment\tline %d\n", lex->line);
          len = _read_multiline_comment(lex, buffer);
          printf("buff = '%s', len = %d\n", buffer, len);
          *pack = candy_pack_string(0, (candy_string_t){buffer, len});
          return CANDY_TOKEN_NULL;
        }
        /* is const string */
        else{
          printf("string\tline %d\n", lex->line);
          len = _read_string(lex, buffer, false);
          printf("buff = '%s', len = %d\n", buffer, len);
          *pack = candy_pack_string(0, (candy_string_t){buffer, len});
          return CANDY_TOKEN_CONST_STRING;
        }
        break;
      default:
        candy_assert(0, "unexpected char");
        break;
    }

  }
  return CANDY_TOKEN_NULL;
}
