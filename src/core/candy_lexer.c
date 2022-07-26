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
  candy_tokens_t token;
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

/**
  * @brief  skip comment, like
  *         # hello world
  * @param  lex lexer
  * @retval reserved
  */
static int _skip_comment(candy_lexer_t lex) {
  char *tail = strchr(lex->curr, '\n');
  candy_assert(tail != NULL, "comment not closed");
  lex->curr = tail;
  return 0;
}

/**
  * @brief  get newline, like
  *         \r
  *         \n
  *         \r\n
  *         \n\r
  * @param  lex lexer
  * @param  buff store buffer
  * @return newline bytes
  */
static int _get_newline(candy_lexer_t lex, char buff[]) {
  char *dst = buff;
  *dst++ = *lex->curr++;
  if (*lex->curr == '\n' || *lex->curr == '\r') {
    *buff++ = *lex->curr++;
  }
  lex->line++;
  return dst - buff;
}

static char _get_intchar(candy_lexer_t lex, uint8_t max_len, uint8_t base) {
  char value = 0;
  char buff[] = {lex->curr[0], lex->curr[1], lex->curr[2], '\0'};
  buff[max_len] = '\0';
  char *end = NULL;
  value = strtoul(buff, &end, base);
  lex->curr += (end - buff);
  return value;
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
static inline char _get_octchar(candy_lexer_t lex) {
  return _get_intchar(lex, 3, 8);
}

/**
  * @brief  gets hexadecimal escape character, like
  *         "30"
  * @param  lex lexer
  * @retval hexadecimal character
  */
static inline char _get_hexchar(candy_lexer_t lex) {
  return _get_intchar(lex, 2, 16);
}

static candy_tokens_t _get_number(candy_lexer_t lex, char buff[], const int sign, candy_meta_t *meta) {
  char *dst = buff;
  bool is_float = false;
  *dst++ = *lex->curr++;
  if ((*lex->curr == 'x' || *lex->curr == 'X')) {
    lex->curr++;
    char *end = NULL;
    meta->i = (candy_integer_t)strtol(lex->curr, &end, 16) * sign;
    candy_assert(end != NULL, "invalid hexadecimal number");
    lex->curr = end;
    return CANDY_TK_CST_INTEGER;
  }
  while (1) {
    switch (*lex->curr) {
      case '0' ... '9':
        *dst++ = *lex->curr++;
        break;
      case '.':
        candy_assert(!is_float, "invalid float");
        is_float = true;
        *dst++ = *lex->curr++;
        break;
      case 'e': case 'E':
        is_float = true;
        *dst++ = *lex->curr++;
        if (*lex->curr == '+' || *lex->curr == '-') {
          *dst++ = *lex->curr++;
        }
        break;
      default:
        *dst = '\0';
        if (is_float) {
          meta->f = (candy_float_t)strtod(buff, NULL) * sign;
          return CANDY_TK_CST_FLOAT;
        }
        else {
          meta->i = (candy_integer_t)strtol(buff, NULL, 10) * sign;
          return CANDY_TK_CST_INTEGER;
        }
    }
  }
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
        dst += _get_newline(lex, dst);
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
          case         'x': lex->curr++; *dst++ =  _get_hexchar(lex); break;
          case '0' ... '7':              *dst++ =  _get_octchar(lex); break;
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

static candy_tokens_t _get_ident_or_keyword(candy_lexer_t lex, char buff[], const char curr, candy_meta_t *meta) {
  char *dst = buff;
  /* save alpha, or '_' */
  *dst++ = curr;
  /* save number, alpha, or '_' */
  while (is_dec(*lex->curr) || is_alpha(*lex->curr) || *lex->curr == '_')
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

static candy_tokens_t _next(candy_lexer_t lex, candy_meta_t *meta) {
  candy_assert(lex != NULL);
  char buffer[CANDY_PARSER_BUFFER_SIZE] = {0};
  int len = 0;
  /* while until *src == '\0' */
  while (1) {
    switch (*lex->curr) {
      case '\0':
        return CANDY_TK_NONE;
      case '\r': case '\n':
        _get_newline(lex, buffer);
        break;
      case ' ': case '\f': case '\t': case '\v':
        lex->curr++;
        break;
      case '+': case '-':
        switch (lex->curr[1]) {
          case '=':
            *buffer = *lex->curr;
            lex->curr += 2;
            /* CANDY_TK_OPE_ADDASS or CANDY_TK_OPE_SUBASS */
            return *buffer + CANDY_TK_MIN;
          case '0' ... '9':
            return _get_number(lex, buffer, ('+' == *lex->curr++) ? (1) : (-1), meta);
          default:
            /* CANDY_TK_OPE_ADD or CANDY_TK_OPE_SUB */
            return *lex->curr++;
        }
      case '*': case '/': case '%':
      case '=': case '>': case '<':
        switch (lex->curr[1]) {
          case '=':
            *buffer = *lex->curr;
            lex->curr += 2;
            return *buffer + CANDY_TK_MIN;
          default:
            return *lex->curr++;
        }
      case '!':
        candy_assert(lex->curr[1] == '=', "unexpected token");
        *buffer = *lex->curr;
        lex->curr += 2;
        /* CANDY_TK_OPE_NEQUAL */
        return *buffer + CANDY_TK_MIN;
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
        return _get_number(lex, buffer, 1, meta);
      default:
        if (is_alpha(*lex->curr) || *lex->curr == '_')
          return _get_ident_or_keyword(lex, buffer, *lex->curr++, meta);
        candy_assert(false, "unexpected char '%c'", *lex->curr);
        break;
    }
  }
}

const char *candy_lexer_get_tokenstr(candy_tokens_t token) {
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

candy_tokens_t candy_lexer_curr(candy_lexer_t lex, candy_meta_t *meta) {
  /* is there a look-ahead token? */
  if (lex->lookahead.token != CANDY_TK_EOS) {
    /* use this one */
    candy_tokens_t token = lex->lookahead.token;
    if (meta != NULL)
      *meta = lex->lookahead.meta;
    /* and discharge it */
    lex->lookahead.token = CANDY_TK_EOS;
    return token;
  }
  /* read next token */
  return _next(lex, meta);
}

candy_tokens_t candy_lexer_lookahead(candy_lexer_t lex) {
  if (lex->lookahead.token == CANDY_TK_EOS)
    lex->lookahead.token = _next(lex, &lex->lookahead.meta);
  return lex->lookahead.token;
}

