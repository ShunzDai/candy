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
#include "candy_wrap.h"
#include "candy_lib.h"
#include "candy_platform.h"
#include <stdlib.h>

struct candy_lexer {
#ifdef CANDY_DEBUG_MODE
  struct {
    uint16_t line;
    uint16_t column;
  } dbg;
#endif /* CANDY_DEBUG_MODE */
  const char *curr;
  const char *error;
  struct {
    uint8_t type;
  } indent;
  struct {
    uint8_t token;
    candy_meta_t meta;
  } lookahead;
  struct candy_view *buffer;
  char *cursor;
};

static const struct {
  candy_tokens_t token;
  const char * const keyword;
} _keywords[] = {
  #define CANDY_KW_MATCH
  #include "candy_keyword.list"
};

static inline char _skip_idx(struct candy_lexer *lex, int idx) {
#ifdef CANDY_DEBUG_MODE
  lex->dbg.column += idx;
#endif /* CANDY_DEBUG_MODE */
  char ch = *lex->curr;
  return lex->curr += idx, ch;
}

static inline char _skip_curr(struct candy_lexer *lex) {
  return _skip_idx(lex, 1);
}

static inline void _save_char(struct candy_lexer *lex, char ch) {
  *lex->cursor++ = ch;
}

static inline void _save_curr(struct candy_lexer *lex) {
  _save_char(lex, _skip_curr(lex));
}

static inline bool _check_dual(struct candy_lexer *lex, const char str[]) {
  if (*lex->curr == str[0] || *lex->curr == str[1]) {
    _save_curr(lex);
    return true;
  }
  return false;
}

/**
  * @brief  get newline, like
  *         \r
  *         \n
  *         \r\n
  *         \n\r
  *         first byte of curr pointer has been checked by up layer function
  * @param  lex lexer
  * @return newline bytes
  */
static int _get_newline(struct candy_lexer *lex) {
  const char *head = lex->cursor;
  _save_curr(lex);
  _check_dual(lex, "\r\n");
#ifdef CANDY_DEBUG_MODE
  lex->dbg.line++;
  lex->dbg.column = 0;
#endif /* CANDY_DEBUG_MODE */
  return lex->cursor - head;
}

/**
  * @brief  skip comment, like
  *         # hello world
  * @param  lex lexer
  * @retval reserved
  */
static int _skip_comment(struct candy_lexer *lex) {
  _skip_curr(lex);
  while (1) {
    switch (*lex->curr) {
      case '\r': case '\n':
        _get_newline(lex);
      case '\0':
        return 0;
      default:
        _skip_curr(lex);
        break;
    }
  }
}

static char _get_intchar(struct candy_lexer *lex, uint8_t max_len, uint8_t base) {
  char buff[] = {lex->curr[0], lex->curr[1], lex->curr[2], '\0'};
  buff[max_len] = '\0';
  char *end = NULL;
  char value = strtoul(buff, &end, base);
  _skip_idx(lex, end - buff);
  return value;
}

/**
  * @brief  gets octal escape character, like
  *         "0"
  *         "01"
  *         "012"
  * @param  lex lexer
  * @retval octal character
  */
static inline char _get_octchar(struct candy_lexer *lex) {
  return _get_intchar(lex, 3, 8);
}

/**
  * @brief  gets hexadecimal escape character, like
  *         "30"
  * @param  lex lexer
  * @retval hexadecimal character
  */
static inline char _get_hexchar(struct candy_lexer *lex) {
  return _get_intchar(lex, 2, 16);
}

static candy_tokens_t _get_number(struct candy_lexer *lex, candy_meta_t *meta) {
  bool is_float = false;
  _save_curr(lex);
  if (*lex->buffer->data == '0' && _check_dual(lex, "xX")) {
    char *end = NULL;
    meta->i = (candy_integer_t)strtol(lex->curr, &end, 16);
    candy_assert(end != NULL, "invalid hexadecimal number");
    _skip_idx(lex, end - lex->curr);
    return CANDY_TK_CST_INTEGER;
  }
  while (1) {
    switch (*lex->curr) {
      case '0' ... '9':
        _save_curr(lex);
        break;
      case '.':
        candy_assert(!is_float, "invalid float");
        is_float = true;
        _save_curr(lex);
        break;
      case 'e': case 'E':
        is_float = true;
        _save_curr(lex);
        _check_dual(lex, "+-");
        break;
      default:
        _save_char(lex, '\0');
        if (is_float) {
          meta->f = (candy_float_t)strtod(lex->buffer->data, NULL);
          return CANDY_TK_CST_FLOAT;
        }
        else {
          meta->i = (candy_integer_t)strtol(lex->buffer->data, NULL, 10);
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
  * @param  multiline is multiline string or not
  * @retval string length, not include '\0'
  */
static int _get_string(struct candy_lexer *lex, bool multiline) {
  const char *head = lex->cursor;
  const char del = *lex->curr;
  /* skip first " or ' */
  _skip_idx(lex, multiline ? 3 : 1);
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
        _get_newline(lex);
        break;
      case '\\':
        _skip_curr(lex);
        switch (*lex->curr) {
          case         'a': _skip_curr(lex); _save_char(lex,              '\a'); break;
          case         'b': _skip_curr(lex); _save_char(lex,              '\b'); break;
          case         't': _skip_curr(lex); _save_char(lex,              '\t'); break;
          case         'n': _skip_curr(lex); _save_char(lex,              '\n'); break;
          case         'v': _skip_curr(lex); _save_char(lex,              '\v'); break;
          case         'f': _skip_curr(lex); _save_char(lex,              '\f'); break;
          case         'r': _skip_curr(lex); _save_char(lex,              '\r'); break;
          case        '\\': _skip_curr(lex); _save_char(lex,              '\\'); break;
          case        '\'': _skip_curr(lex); _save_char(lex,              '\''); break;
          case         '"': _skip_curr(lex); _save_char(lex,               '"'); break;
          case         'x': _skip_curr(lex); _save_char(lex, _get_hexchar(lex)); break;
          case '0' ... '7':                  _save_char(lex, _get_octchar(lex)); break;
          /* todo: support unicode */
          default:
            /* is not escape */
            // printf("unknown escape sequence: '\\%c'\n", *lex->curr);
            _save_char(lex, '\\');
            _save_curr(lex);
            break;
        }
        break;
      /* normal character */
      default:
        _save_curr(lex);
        break;
    }
  }
  _save_char(lex, '\0');
  /* skip last " or ' */
  candy_assert(*lex->curr == del && (multiline ? (lex->curr[1] == del && lex->curr[2] == del) : (true)), "unexpected end of string");
  _skip_idx(lex, multiline ? 3 : 1);
  return lex->cursor - head - 1;
}

static candy_tokens_t _get_ident_or_keyword(struct candy_lexer *lex, candy_meta_t *meta) {
  /* save alpha, or '_' */
  _save_curr(lex);
  /* save number, alpha, or '_' */
  while (is_dec(*lex->curr) || is_alpha(*lex->curr) || *lex->curr == '_')
    _save_curr(lex);
  /* last char padding '\0' */
  _save_char(lex, '\0');
  /* check keyword */
  for (unsigned i = 0; i < candy_lengthof(_keywords); i++) {
    if (strcmp(lex->buffer->data, _keywords[i].keyword) == 0)
      return _keywords[i].token;
  }
  meta->hash = candy_hash(lex->buffer->data);
  return CANDY_TK_IDENT;
}

static candy_tokens_t _get_next_token(struct candy_lexer *lex, candy_meta_t *meta) {
  lex->cursor = lex->buffer->data;
  int len = 0;
  while (1) {
    switch (*lex->curr) {
      case '\0':
        return CANDY_TK_NONE;
      case '\r': case '\n':
        _get_newline(lex);
        break;
      case ' ': case '\f': case '\t': case '\v':
        _skip_curr(lex);
        break;
      case '!':
        candy_assert(lex->curr[1] == '=', "unexpected token '%5.5s'", *lex->curr);
        goto dual_ope;
      case '+': case '-':
        if (lex->curr[1] == '=')
          goto dual_ope;
        return _skip_curr(lex);
      case '>': case '<': case '*': case '/':
        if (lex->curr[1] == *lex->curr)
          goto dual_ope;
      case '%': case '=':
        if (lex->curr[1] == '=')
          goto dual_ope;
      case '&': case '|': case '~': case '^':
      case '(': case ')': case '[': case ']':
        return _skip_curr(lex);
      /* is singleline comment */
      case '#':
        _skip_comment(lex);
        break;
      /* is string */
      case '"': case '\'':
        len = _get_string(lex, lex->curr[1] == *lex->curr && lex->curr[2] == *lex->curr);
        meta->wrap = candy_wrap_string(0, lex->buffer->data, len);
        return CANDY_TK_CST_STRING;
      case '0' ... '9':
        return _get_number(lex, meta);
      default:
        if (is_alpha(*lex->curr) || *lex->curr == '_')
          return _get_ident_or_keyword(lex, meta);
        candy_assert(false, "unexpected token '%5.5s'", *lex->curr);
        break;
    }
  }
  dual_ope:
  return tk_dual_ope(_skip_curr(lex), _skip_curr(lex));
}

struct candy_lexer *candy_lexer_create(const char code[], struct candy_view *buffer) {
  struct candy_lexer *lex = (struct candy_lexer *)malloc(sizeof(struct candy_lexer));
#ifdef CANDY_DEBUG_MODE
  lex->dbg.line = 1;
  lex->dbg.column = 0;
#endif /* CANDY_DEBUG_MODE */
  lex->curr = code;
  lex->lookahead.token = CANDY_TK_EOS;
  lex->lookahead.meta.data = 0;
  lex->buffer = buffer;
  return lex;
}

int candy_lexer_delete(struct candy_lexer **lex) {
  free(*lex);
  *lex = NULL;
  return 0;
}

candy_tokens_t candy_lexer_curr(struct candy_lexer *lex, candy_meta_t *meta) {
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
  return _get_next_token(lex, meta);
}

candy_tokens_t candy_lexer_lookahead(struct candy_lexer *lex) {
  if (lex->lookahead.token == CANDY_TK_EOS)
    lex->lookahead.token = _get_next_token(lex, &lex->lookahead.meta);
  return lex->lookahead.token;
}


