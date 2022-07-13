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
#include "candy_lexer.h"
#include "src/common/candy_lib.h"
#include "src/platform/candy_memory.h"
#include "src/struct/candy_wrap.h"
#include "candy_config.h"
#include <stdlib.h>

struct candy_lexer {
  const char *code;
  const char *curr;
  const char *error;
  int8_t indent;
  uint32_t line;

  struct lookahead{
    int8_t token;
    candy_meta_t meta;
  } lookahead;
};

static const struct {
  const char * const keyword;
  int8_t token;
} _keywords[] = {
  {"False",     CANDY_TK_KW_FALSE},
  {"True",      CANDY_TK_KW_TRUE},
  {"and",       CANDY_TK_KW_AND},
  {"or",        CANDY_TK_KW_OR},
  {"not",       CANDY_TK_KW_NOT},
  {"if",        CANDY_TK_KW_IF},
  {"else",      CANDY_TK_KW_ELIF},
  {"elif",      CANDY_TK_KW_ELSE},
  {"while",     CANDY_TK_KW_WHILE},
  {"for",       CANDY_TK_KW_FOR},
  {"break",     CANDY_TK_KW_BREAK},
  {"continue",  CANDY_TK_KW_CONTINUE},
};

static inline bool _is_capital(char ch) {
  return ch >= 'A' && ch <= 'Z';
}

static inline bool _is_lower(char ch) {
  return ch >= 'a' && ch <= 'z';
}

static inline bool _is_alpha(char ch) {
  return _is_capital(ch) || _is_lower(ch);
}

static inline bool _is_oct(char ch) {
  return ch >= '0' && ch <= '7';
}

static inline bool _is_dec(char ch) {
  return ch >= '0' && ch <= '9';
}

static inline bool _is_hex(char ch) {
  return (_is_dec(ch) || (ch >= 'a' && ch <= 'f')) || (ch >= 'A' && ch <= 'F');
}

/**
  * @brief  skip newline, like
  *         \r
  *         \n
  *         \r\n
  *         \n\r
  * @param  lex lexer
  * @return skip bytes
  */
static int _skip_newline(candy_lexer_t lex, char buff[]) {
  char *dst = buff;
  *dst++ = *lex->curr++;
  if (*lex->curr == '\n' || *lex->curr == '\r') {
    *buff++ = *lex->curr++;
  }
  lex->line++;
  return dst - buff;
}

/**
  * @brief  skip comment, like
  *         # hello world
  * @param  lex lexer
  * @param  buff store buffer
  * @retval reserved
  */
static int _skip_comment(candy_lexer_t lex) {
  char *tail = strchr(lex->curr, '\n');
  candy_assert(tail != NULL, "comment not closed");
  lex->curr = tail;
  return 0;
}

/**
  * @brief  gets hexadecimal escape character, like
  *         "30"
  * @param  lex lexer
  * @retval hexadecimal character
  */
static char _get_hexchar(candy_lexer_t lex) {
  /* save hex */
  char hex[] = {*lex->curr, lex->curr[1], '\0'};
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
  *         first byte of curr pointer has been checked by the function @ref _read_string
  * @param  lex lexer
  * @retval octal character
  */
static char _get_octchar(candy_lexer_t lex) {
  /* save oct */
  char oct[] = {*lex->curr, lex->curr[1], lex->curr[2], '\0'};
  lex->curr += 3;
  if (_is_oct(oct[1]) == false) {
    oct[1] = '\0';
    lex->curr -= 2;
  }
  else if (_is_oct(oct[2]) == false) {
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
static int _get_string(candy_lexer_t lex, char buff[], bool multiline) {
  candy_assert(buff != NULL, "buffer is null");
  const char del = *lex->curr;
  char *dst = buff;
  /* skip first " or ' */
  lex->curr += multiline ? 3 : 1;
  while (*lex->curr != del) {
    switch (*lex->curr) {
      case '\0':
        candy_assert(false, "unexpected end of string");
        return -1;
      case '\r': case '\n':
        if (multiline == false) {
          candy_assert(false, "single line string can not contain newline");
          return -1;
        }
        dst += _skip_newline(lex, dst);
        break;
      case '\\':
        lex->curr++;
        switch (*lex->curr) {
          case         'a': lex->curr++; *dst++ =               '\a'; break;
          case         'b': lex->curr++; *dst++ =               '\b'; break;
          case         't': lex->curr++; *dst++ =               '\t'; break;
          case         'n': lex->curr++; *dst++ =               '\n'; break;
          case         'v': lex->curr++; *dst++ =               '\v'; break;
          case         'f': lex->curr++; *dst++ =               '\f'; break;
          case         'r': lex->curr++; *dst++ =               '\r'; break;
          case        '\\': lex->curr++; *dst++ =               '\\'; break;
          case        '\'': lex->curr++; *dst++ =               '\''; break;
          case         '"': lex->curr++; *dst++ =                '"'; break;
          case         'x': lex->curr++; *dst++ = _get_hexchar(lex); break;
          case '0' ... '7':              *dst++ = _get_octchar(lex); break;
          /* todo: support unicode */
          default:
            /* is not escape */
            // printf("unknown escape sequence: '\\%c'\n", *lex->curr);
            *dst++ = '\\';
            *dst++ = *lex->curr++;
            break;
        }
        break;
      /* normal character */
      default:
        *dst++ = *lex->curr++;
        break;
    }
  }
  *dst = '\0';
  /* skip last " or ' */
  candy_assert(*lex->curr == del && (multiline ? (lex->curr[1] == del && lex->curr[2] == del) : (true)), "unexpected end of string");
  lex->curr += multiline ? 3 : 1;
  return dst - buff;
}

static int8_t _get_number(candy_lexer_t lex, char buff[], candy_meta_t *meta) {
  bool f = false;
  /* save integer part */
  char *dst = buff;
  while (_is_dec(*lex->curr)) {
    *dst++ = *lex->curr++;
  }
  /* save fraction part */
  if (*lex->curr == '.') {
    f = true;
    *dst++ = *lex->curr++;
    while (_is_dec(*lex->curr)) {
      *dst++ = *lex->curr++;
    }
  }
  /* save exponent part */
  if (*lex->curr == 'e' || *lex->curr == 'E') {
    *dst++ = *lex->curr++;
    if (*lex->curr == '+' || *lex->curr == '-') {
      *dst++ = *lex->curr++;
    }
    while (_is_dec(*lex->curr)) {
      *dst++ = *lex->curr++;
    }
  }
  *dst = '\0';
  /* convert to float or int */
  if (f) {
    /* is float */
    meta->f = strtof(buff, NULL);
    return CANDY_TK_CST_FLOAT;

  }
  /* is integer */
  meta->i = strtol(buff, NULL, 10);
  return CANDY_TK_CST_INTEGER;
}

static int8_t _get_ident_or_keyword(candy_lexer_t lex, char buff[], candy_meta_t *meta) {
  char *dst = buff;
  /* save alpha, or '_' */
  *dst++ = *lex->curr++;
  /* save number, alpha, or '_' */
  while (_is_dec(*lex->curr) || _is_alpha(*lex->curr) || *lex->curr == '_')
    *dst++ = *lex->curr++;
  /* last char padding '\0' */
  *dst = '\0';
  /* check keyword */
  for (unsigned i = 0; i < candy_lengthof(_keywords); i++) {
    if (strcmp(buff, _keywords[i].keyword) == 0)
      return _keywords[i].token;
  }
  meta->hash = candy_hash(buff);
  return CANDY_TK_IDENT;
}

static int8_t _next(candy_lexer_t lex, candy_meta_t *meta) {
  candy_assert(lex != NULL);
  char buffer[CANDY_PARSER_BUFFER_SIZE] = {0};
  int len = 0;
  /* while until *src == '\0' */
  while (1) {
    switch (*lex->curr) {
      case '\0':
        return CANDY_TK_NONE;
      case '\r': case '\n':
        _skip_newline(lex, buffer);
        break;
      case ' ': case '\f': case '\t': case '\v':
        lex->curr++;
        break;
      case '+': case '-': case '*': case '/': case '%':
      case '=': case '!': case '>': case '<':
        if (*lex->curr == '!')
          candy_assert(lex->curr[1] == '=', "unexpected token");
        if (lex->curr[1] == '=') {
          char ch = *lex->curr;
          lex->curr += 2;
          return ch + CANDY_TK_MIN;
        }
        return *lex->curr++;
      case '(': case ')':
        return *lex->curr++;
      /* is singleline comment */
      case '#':
        _skip_comment(lex);
        break;
      /* is string */
      case '"': case '\'':
        len = _get_string(lex, buffer, lex->curr[1] == *lex->curr && lex->curr[2] == *lex->curr);
        meta->wrap = candy_wrap_string(0, buffer, len);
        return CANDY_TK_CST_STRING;
      case '0' ... '9':
        return _get_number(lex, buffer, meta);
      default:
        if (_is_alpha(*lex->curr) || *lex->curr == '_')
          return _get_ident_or_keyword(lex, buffer, meta);
        candy_assert(false, "unexpected char '%c'", *lex->curr);
        break;
    }
  }
}

const char *candy_lexer_get_tokenstr(int8_t token) {
  for (unsigned i = 0; i < candy_lengthof(_keywords); i++) {
    if (_keywords[i].token == token)
      return _keywords[i].keyword;
  }
  return NULL;
}

candy_lexer_t candy_lexer_create(const char code[]) {
  candy_lexer_t lex = (candy_lexer_t)candy_malloc(sizeof(struct candy_lexer));
  lex->code = code;
  lex->curr = code;
  lex->line = 1;
  lex->lookahead.token = CANDY_TK_EOS;
  return lex;
}

int candy_lexer_delete(candy_lexer_t *lex) {
  candy_free(*lex);
  *lex = NULL;
  return 0;
}

int8_t candy_lexer_curr(candy_lexer_t lex, candy_meta_t *meta) {
  /* is there a look-ahead token? */
  if (lex->lookahead.token != CANDY_TK_EOS) {
    /* use this one */
    int8_t token = lex->lookahead.token;
    if (meta != NULL)
      *meta = lex->lookahead.meta;
    /* and discharge it */
    lex->lookahead.token = CANDY_TK_EOS;
    return token;
  }
  /* read next token */
  return _next(lex, meta);
}

int8_t candy_lexer_lookahead(candy_lexer_t lex) {
  if (lex->lookahead.token == CANDY_TK_EOS)
    lex->lookahead.token = _next(lex, &lex->lookahead.meta);
  return lex->lookahead.token;
}

