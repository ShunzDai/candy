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
#include "src/candy_io.h"
#include "src/candy_wrap.h"
#include "src/candy_lib.h"
#include "src/candy_platform.h"
#include <stdlib.h>

struct candy_lexer {
  candy_io_t io;
#ifdef CANDY_DEBUG_MODE
  struct {
    uint16_t line;
    uint16_t column;
  } dbg;
#endif /* CANDY_DEBUG_MODE */
  struct {
    uint8_t type;
  } indent;
  struct {
    uint8_t token;
    candy_wrap_t wrap;
  } lookahead;
};

static const struct {
  candy_tokens_t token;
  const char * const keyword;
} _keywords[] = {
  #define CANDY_KW_MATCH
  #include "src/candy_keyword.list"
};

/**
  * @brief  observer mode of the stream
  * @param  self lexer
  * @param  idx  lookahead to the idx byte, idx value between 0 and @ref CANDY_IO_LOOKAHEAD_SIZE - 1
  * @retval target byte
  */
static inline char _view(candy_lexer_t *self, int idx) {
  return candy_io_view(&self->io, idx);
}

/**
  * @brief  read out the next byte of the stream
  * @param  self lexer
  * @retval target byte
  */
static inline char _read(candy_lexer_t *self) {
  #ifdef CANDY_DEBUG_MODE
  ++self->dbg.column;
  #endif /* CANDY_DEBUG_MODE */
  return candy_io_read(&self->io);
}

/**
  * @brief  skipping n bytes of the stream
  * @param  self lexer
  * @param  n    count of bytes
  * @retval none
  */
static inline void _skip(candy_lexer_t *self, int n) {
  while (n--)
    _read(self);
}

/**
  * @brief  store a byte
  * @param  self lexer
  * @param  ch   byte to be stored
  * @retval none
  */
static inline void _save_char(candy_lexer_t *self, char ch) {
  candy_io_write(&self->io, ch);
}

/**
  * @brief  store the next byte of the stream
  * @param  self lexer
  * @retval none
  */
static inline void _save(candy_lexer_t *self) {
  _save_char(self, _read(self));
}

/**
  * @brief  checks whether the next byte of the stream is one of the two bytes the user expects,
  *         if so, the byte will be stored
  * @param  self lexer
  * @param  str  the string that the user expects
  * @retval boolean
  */
static inline bool _check_dual(candy_lexer_t *self, const char str[]) {
  if (_view(self, 0) == str[0] || _view(self, 0) == str[1]) {
    _save(self);
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
  * @param  self lexer
  * @return newline bytes
  */
static int _get_newline(candy_lexer_t *self) {
  int head = self->io.w;
  _save(self);
  _check_dual(self, "\r\n");
#ifdef CANDY_DEBUG_MODE
  self->dbg.line++;
  self->dbg.column = 0;
#endif /* CANDY_DEBUG_MODE */
  return self->io.w - head;
}

/**
  * @brief  skip comment, like
  *         # hello world
  * @param  self lexer
  * @retval reserved
  */
static int _skip_comment(candy_lexer_t *self) {
  _read(self);
  while (1) {
    switch (_view(self, 0)) {
      case '\r': case '\n':
        _get_newline(self);
      case '\0':
        return 0;
      default:
        _read(self);
        break;
    }
  }
}

static char _get_intchar(candy_lexer_t *self, uint8_t max_len, uint8_t base) {
  char buff[] = {_view(self, 0), _view(self, 1), _view(self, 2), '\0'};
  buff[max_len] = '\0';
  char *end = NULL;
  char value = strtoul(buff, &end, base);
  _skip(self, end - buff);
  return value;
}

/**
  * @brief  gets octal escape character, like
  *         "0"
  *         "01"
  *         "012"
  * @param  self lexer
  * @retval octal character
  */
static inline char _get_octchar(candy_lexer_t *self) {
  return _get_intchar(self, 3, 8);
}

/**
  * @brief  gets hexadecimal escape character, like
  *         "30"
  * @param  self lexer
  * @retval hexadecimal character
  */
static inline char _get_hexchar(candy_lexer_t *self) {
  return _get_intchar(self, 2, 16);
}

static candy_tokens_t _get_number(candy_lexer_t *self, candy_wrap_t *wrap) {
  bool is_float = false;
  _save(self);
  if (*self->io.buffer == '0' && _check_dual(self, "xX")) {
    while (is_hex(_view(self, 0)))
      _save(self);
    char *end = NULL;
    candy_integer_t i = (candy_integer_t)strtol(self->io.buffer, &end, 16);
    candy_assert(end != NULL, "invalid hexadecimal number");
    candy_wrap_init_integer(wrap, &i, 1);
    return CANDY_TK_CST_INTEGER;
  }
  while (1) {
    switch (_view(self, 0)) {
      case '0' ... '9':
        _save(self);
        break;
      case '.':
        candy_assert(!is_float, "invalid float");
        is_float = true;
        _save(self);
        break;
      case 'e': case 'E':
        is_float = true;
        _save(self);
        _check_dual(self, "+-");
        break;
      default:
        if (is_float) {
          candy_float_t f = (candy_float_t)strtod(self->io.buffer, NULL);
          candy_wrap_init_float(wrap, &f, 1);
          return CANDY_TK_CST_FLOAT;
        }
        else {
          candy_integer_t i = (candy_integer_t)strtol(self->io.buffer, NULL, 10);
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
  * @param  self lexer
  * @param  multiline is multiline string or not
  * @retval string length, not include '\0'
  */
static int _get_string(candy_lexer_t *self, const bool multiline) {
  int head = self->io.w;
  const char del = _view(self, 0);
  /* skip first " or ' */
  _skip(self, multiline ? 3 : 1);
  while (_view(self, 0) != del) {
    switch (_view(self, 0)) {
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
        _read(self);
        switch (_view(self, 0)) {
          case         'a': _read(self); _save_char(self,               '\a'); break;
          case         'b': _read(self); _save_char(self,               '\b'); break;
          case         't': _read(self); _save_char(self,               '\t'); break;
          case         'n': _read(self); _save_char(self,               '\n'); break;
          case         'v': _read(self); _save_char(self,               '\v'); break;
          case         'f': _read(self); _save_char(self,               '\f'); break;
          case         'r': _read(self); _save_char(self,               '\r'); break;
          case        '\\': _read(self); _save_char(self,               '\\'); break;
          case        '\'': _read(self); _save_char(self,               '\''); break;
          case         '"': _read(self); _save_char(self,                '"'); break;
          case         'x': _read(self); _save_char(self, _get_hexchar(self)); break;
          case '0' ... '7':              _save_char(self, _get_octchar(self)); break;
          /* todo: support unicode */
          default:
            /* is not escape */
            // printf("unknown escape sequence: '\\%c'\n", _view(self, 0));
            _save_char(self, '\\');
            _save(self);
            break;
        }
        break;
      /* normal character */
      default:
        _save(self);
        break;
    }
  }
  /* skip last " or ' */
  candy_assert(_view(self, 0) == del && (multiline ? (_view(self, 1) == del && _view(self, 2) == del) : (true)), "unexpected end of string");
  _skip(self, multiline ? 3 : 1);
  return self->io.w - head;
}

static candy_tokens_t _get_ident_or_keyword(candy_lexer_t *self, candy_wrap_t *wrap) {
  /* save alpha, or '_' */
  _save(self);
  /* save number, alpha, or '_' */
  while (is_dec(_view(self, 0)) || is_alpha(_view(self, 0)) || _view(self, 0) == '_')
    _save(self);
  /* check keyword */
  for (unsigned i = 0; i < candy_lengthof(_keywords); i++) {
    if (strncmp(self->io.buffer, _keywords[i].keyword, self->io.w) == 0)
      return _keywords[i].token;
  }
  // wrap->hash = djb_hash2(self->buffer->data, self->io.w);
  return CANDY_TK_IDENT;
}

static candy_tokens_t _lexer(candy_lexer_t *self, candy_wrap_t *wrap) {
  self->io.w = 0;
  int len = 0;
  while (1) {
    switch (_view(self, 0)) {
      case '\0':
        return CANDY_TK_NONE;
      case '\r': case '\n':
        _get_newline(self);
        break;
      case ' ': case '\f': case '\t': case '\v':
        _read(self);
        break;
      case '!':
        candy_assert(_view(self, 1) == '=', "unexpected token '%5.5s'", _view(self, 0));
        goto dual_ope;
      case '+': case '-':
        if (_view(self, 1) == '=')
          goto dual_ope;
        return _read(self);
      case '>': case '<': case '*': case '/':
        if (_view(self, 1) == _view(self, 0))
          goto dual_ope;
      case '%': case '=':
        if (_view(self, 1) == '=')
          goto dual_ope;
      case '&': case '|': case '~': case '^':
      case '(': case ')': case '[': case ']':
      case ':':
        return _read(self);
      /* is singleline comment */
      case '#':
        _skip_comment(self);
        break;
      /* is string */
      case '"': case '\'':
        len = _get_string(self, _view(self, 1) == _view(self, 0) && _view(self, 2) == _view(self, 0));
        candy_wrap_init_string(wrap, self->io.buffer, len);
        return CANDY_TK_CST_STRING;
      case '0' ... '9':
        return _get_number(self, wrap);
      default:
        if (is_alpha(_view(self, 0)) || _view(self, 0) == '_')
          return _get_ident_or_keyword(self, wrap);
        candy_assert(false, "unexpected token '%c'", _view(self, 0));
        break;
    }
  }
  dual_ope:
  return tk_dual_ope(_read(self), _read(self));
}

candy_lexer_t *candy_lexer_create(candy_reader_t reader, void *ud) {
  candy_lexer_t *self = (candy_lexer_t *)malloc(sizeof(struct candy_lexer));
  candy_io_init(&self->io, reader, ud);
#ifdef CANDY_DEBUG_MODE
  self->dbg.line = 1;
  self->dbg.column = 0;
#endif /* CANDY_DEBUG_MODE */
  self->lookahead.token = CANDY_TK_EOS;
  return self;
}

int candy_lexer_delete(candy_lexer_t **self) {
  candy_io_deinit(&(*self)->io);
  free(*self);
  *self = NULL;
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
