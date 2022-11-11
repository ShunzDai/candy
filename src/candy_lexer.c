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
#include "src/candy_lexer.h"
#include "src/candy_wrap.h"
#include "src/candy_lib.h"
#include "src/candy_platform.h"
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
    candy_wrap_t wrap;
  } lookahead;
  struct candy_view *buffer;
  char *cursor;
};

static const struct {
  candy_tokens_t token;
  const char * const keyword;
} _keywords[] = {
  #define CANDY_KW_MATCH
  #include "src/candy_keyword.list"
};

static inline char _skip_idx(candy_lexer_t *self, int idx) {
#ifdef CANDY_DEBUG_MODE
  self->dbg.column += idx;
#endif /* CANDY_DEBUG_MODE */
  char ch = *self->curr;
  return self->curr += idx, ch;
}

static inline char _skip_curr(candy_lexer_t *self) {
  return _skip_idx(self, 1);
}

static inline void _save_char(candy_lexer_t *self, char ch) {
  *self->cursor++ = ch;
}

static inline void _save_curr(candy_lexer_t *self) {
  _save_char(self, _skip_curr(self));
}

static inline bool _check_dual(candy_lexer_t *self, const char str[]) {
  if (*self->curr == str[0] || *self->curr == str[1]) {
    _save_curr(self);
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
static int _get_newline(candy_lexer_t *self) {
  const char *head = self->cursor;
  _save_curr(self);
  _check_dual(self, "\r\n");
#ifdef CANDY_DEBUG_MODE
  self->dbg.line++;
  self->dbg.column = 0;
#endif /* CANDY_DEBUG_MODE */
  return self->cursor - head;
}

/**
  * @brief  skip comment, like
  *         # hello world
  * @param  lex lexer
  * @retval reserved
  */
static int _skip_comment(candy_lexer_t *self) {
  _skip_curr(self);
  while (1) {
    switch (*self->curr) {
      case '\r': case '\n':
        _get_newline(self);
      case '\0':
        return 0;
      default:
        _skip_curr(self);
        break;
    }
  }
}

static char _get_intchar(candy_lexer_t *self, uint8_t max_len, uint8_t base) {
  char buff[] = {self->curr[0], self->curr[1], self->curr[2], '\0'};
  buff[max_len] = '\0';
  char *end = NULL;
  char value = strtoul(buff, &end, base);
  _skip_idx(self, end - buff);
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
static inline char _get_octchar(candy_lexer_t *self) {
  return _get_intchar(self, 3, 8);
}

/**
  * @brief  gets hexadecimal escape character, like
  *         "30"
  * @param  lex lexer
  * @retval hexadecimal character
  */
static inline char _get_hexchar(candy_lexer_t *self) {
  return _get_intchar(self, 2, 16);
}

static candy_tokens_t _get_number(candy_lexer_t *self, candy_wrap_t *wrap) {
  bool is_float = false;
  _save_curr(self);
  if (*self->buffer->data == '0' && _check_dual(self, "xX")) {
    char *end = NULL;
    candy_integer_t i = (candy_integer_t)strtol(self->curr, &end, 16);
    candy_wrap_init_integer(wrap, &i, 1);
    candy_assert(end != NULL, "invalid hexadecimal number");
    _skip_idx(self, end - self->curr);
    return CANDY_TK_CST_INTEGER;
  }
  while (1) {
    switch (*self->curr) {
      case '0' ... '9':
        _save_curr(self);
        break;
      case '.':
        candy_assert(!is_float, "invalid float");
        is_float = true;
        _save_curr(self);
        break;
      case 'e': case 'E':
        is_float = true;
        _save_curr(self);
        _check_dual(self, "+-");
        break;
      default:
        _save_char(self, '\0');
        if (is_float) {
          candy_float_t f = (candy_float_t)strtod(self->buffer->data, NULL);
          candy_wrap_init_float(wrap, &f, 1);
          return CANDY_TK_CST_FLOAT;
        }
        else {
          candy_integer_t i = (candy_integer_t)strtol(self->buffer->data, NULL, 10);
          candy_wrap_init_integer(wrap, &i, 1);
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
static int _get_string(candy_lexer_t *self, bool multiline) {
  const char *head = self->cursor;
  const char del = *self->curr;
  /* skip first " or ' */
  _skip_idx(self, multiline ? 3 : 1);
  while (*self->curr != del) {
    switch (*self->curr) {
      case '\0':
        candy_assert(false, "unexpected end of string");
        return -1;
      case '\r': case '\n':
        if (multiline == false) {
          candy_assert(false, "single line string can not contain newline");
          return -1;
        }
        _get_newline(self);
        break;
      case '\\':
        _skip_curr(self);
        switch (*self->curr) {
          case         'a': _skip_curr(self); _save_char(self,               '\a'); break;
          case         'b': _skip_curr(self); _save_char(self,               '\b'); break;
          case         't': _skip_curr(self); _save_char(self,               '\t'); break;
          case         'n': _skip_curr(self); _save_char(self,               '\n'); break;
          case         'v': _skip_curr(self); _save_char(self,               '\v'); break;
          case         'f': _skip_curr(self); _save_char(self,               '\f'); break;
          case         'r': _skip_curr(self); _save_char(self,               '\r'); break;
          case        '\\': _skip_curr(self); _save_char(self,               '\\'); break;
          case        '\'': _skip_curr(self); _save_char(self,               '\''); break;
          case         '"': _skip_curr(self); _save_char(self,                '"'); break;
          case         'x': _skip_curr(self); _save_char(self, _get_hexchar(self)); break;
          case '0' ... '7':                   _save_char(self, _get_octchar(self)); break;
          /* todo: support unicode */
          default:
            /* is not escape */
            // printf("unknown escape sequence: '\\%c'\n", *self->curr);
            _save_char(self, '\\');
            _save_curr(self);
            break;
        }
        break;
      /* normal character */
      default:
        _save_curr(self);
        break;
    }
  }
  _save_char(self, '\0');
  /* skip last " or ' */
  candy_assert(*self->curr == del && (multiline ? (self->curr[1] == del && self->curr[2] == del) : (true)), "unexpected end of string");
  _skip_idx(self, multiline ? 3 : 1);
  return self->cursor - head - 1;
}

static candy_tokens_t _get_ident_or_keyword(candy_lexer_t *self, candy_wrap_t *wrap) {
  /* save alpha, or '_' */
  _save_curr(self);
  /* save number, alpha, or '_' */
  while (is_dec(*self->curr) || is_alpha(*self->curr) || *self->curr == '_')
    _save_curr(self);
  /* last char padding '\0' */
  _save_char(self, '\0');
  /* check keyword */
  for (unsigned i = 0; i < candy_lengthof(_keywords); i++) {
    if (strcmp(self->buffer->data, _keywords[i].keyword) == 0)
      return _keywords[i].token;
  }
  // wrap->hash = candy_hash(self->buffer->data);
  return CANDY_TK_IDENT;
}

static candy_tokens_t _lexer(candy_lexer_t *self, candy_wrap_t *wrap) {
  self->cursor = self->buffer->data;
  int len = 0;
  while (1) {
    switch (*self->curr) {
      case '\0':
        return CANDY_TK_NONE;
      case '\r': case '\n':
        _get_newline(self);
        break;
      case ' ': case '\f': case '\t': case '\v':
        _skip_curr(self);
        break;
      case '!':
        candy_assert(self->curr[1] == '=', "unexpected token '%5.5s'", *self->curr);
        goto dual_ope;
      case '+': case '-':
        if (self->curr[1] == '=')
          goto dual_ope;
        return _skip_curr(self);
      case '>': case '<': case '*': case '/':
        if (self->curr[1] == *self->curr)
          goto dual_ope;
      case '%': case '=':
        if (self->curr[1] == '=')
          goto dual_ope;
      case '&': case '|': case '~': case '^':
      case '(': case ')': case '[': case ']':
        return _skip_curr(self);
      /* is singleline comment */
      case '#':
        _skip_comment(self);
        break;
      /* is string */
      case '"': case '\'':
        len = _get_string(self, self->curr[1] == *self->curr && self->curr[2] == *self->curr);
        candy_wrap_init_string(wrap, self->buffer->data, len);
        return CANDY_TK_CST_STRING;
      case '0' ... '9':
        return _get_number(self, wrap);
      default:
        if (is_alpha(*self->curr) || *self->curr == '_')
          return _get_ident_or_keyword(self, wrap);
        candy_assert(false, "unexpected token '%5.5s'", *self->curr);
        break;
    }
  }
  dual_ope:
  return tk_dual_ope(_skip_curr(self), _skip_curr(self));
}

candy_lexer_t *candy_lexer_create(const char code[], struct candy_view *buffer) {
  candy_lexer_t *self = (candy_lexer_t *)malloc(sizeof(struct candy_lexer));
#ifdef CANDY_DEBUG_MODE
  self->dbg.line = 1;
  self->dbg.column = 0;
#endif /* CANDY_DEBUG_MODE */
  self->curr = code;
  self->lookahead.token = CANDY_TK_EOS;
  self->buffer = buffer;
  return self;
}

int candy_lexer_delete(candy_lexer_t **lex) {
  free(*lex);
  *lex = NULL;
  return 0;
}

candy_tokens_t candy_lexer_next(candy_lexer_t *self, candy_wrap_t *wrap) {
  /* is there a look-ahead token? */
  if (self->lookahead.token != CANDY_TK_EOS) {
    /* use this one */
    candy_tokens_t token = self->lookahead.token;
    if (wrap != NULL)
      *wrap = self->lookahead.wrap;
    /* and discharge it */
    self->lookahead.token = CANDY_TK_EOS;
    return token;
  }
  /* read next token */
  return _lexer(self, wrap);
}

candy_tokens_t candy_lexer_lookahead(candy_lexer_t *self) {
  if (self->lookahead.token == CANDY_TK_EOS)
    self->lookahead.token = _lexer(self, &self->lookahead.wrap);
  return self->lookahead.token;
}
