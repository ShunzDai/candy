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
#include "src/candy_lib.h"
#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>

static const struct {
  candy_tokens_t token;
  const char * const keyword;
} _keywords[] = {
  #define CANDY_KW_MATCH
  #include "src/candy_keyword.list"
};

static inline char *_buffer(candy_lexer_t *self) {
  return self->io->buffer;
}

/**
  * @brief  observer mode of the stream
  * @param  self lexer
  * @param  idx  lookahead to the idx byte, idx value between 0 and @ref CANDY_IO_LOOKAHEAD_SIZE - 1
  * @retval target byte
  */
static inline char _view(candy_lexer_t *self, int idx) {
  return candy_io_view(self->io, idx);
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
  return candy_io_read(self->io);
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
  candy_io_write(self->io, ch);
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
  *         first byte has been checked by up layer function
  * @param  self lexer
  * @return none
  */
static void _skip_newline(candy_lexer_t *self) {
  _save(self);
  _check_dual(self, "\r\n");
#ifdef CANDY_DEBUG_MODE
  self->dbg.line++;
  self->dbg.column = 0;
#endif /* CANDY_DEBUG_MODE */
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
        _skip_newline(self);
      case '\0':
        return 0;
      default:
        _read(self);
        break;
    }
  }
}

/**
  * @brief  gets octal escape character, like
  *         "0"
  *         "01"
  *         "012"
  * @param  self lexer
  * @retval octal character
  */
static inline char _get_octch(candy_lexer_t *self) {
  char buff[] = {_view(self, 0), _view(self, 1), _view(self, 2), '\0'};
  char *end = NULL;
  char value = strtoul(buff, &end, 8);
  _skip(self, end - buff);
  return value;
}

/**
  * @brief  gets hexadecimal escape character, like
  *         "30"
  * @param  self lexer
  * @retval hexadecimal character
  */
static inline char _get_hexch(candy_lexer_t *self) {
  lex_assert(is_hex(_view(self, 0)) && is_hex(_view(self, 1)), "invalid hexadecimal escape");
  return ch2hex(_read(self)) << 4 | ch2hex(_read(self));
}

static candy_tokens_t _get_number(candy_lexer_t *self, candy_wrap_t *wrap) {
  bool is_float = false;
  _save(self);
  if (*_buffer(self) == '0' && _check_dual(self, "xX")) {
    while (is_hex(_view(self, 0)))
      _save(self);
    _save_char(self, '\0');
    candy_integer_t i = (candy_integer_t)strtol(_buffer(self), NULL, 16);
    candy_wrap_init_integer(wrap, &i, 1);
    return CANDY_TK_CST_INTEGER;
  }
  while (1) {
    switch (_view(self, 0)) {
      case '0' ... '9':
        _save(self);
        break;
      case '.':
        lex_assert(!is_float, "multiple dots appear");
        _save(self);
        lex_assert(is_dec(_view(self, 0)), "unknown character '%c'(0x%02X)", _view(self, 0), _view(self, 0));
        is_float = true;
        break;
      case 'e': case 'E':
        _save(self);
        _check_dual(self, "+-");
        lex_assert(is_dec(_view(self, 0)), "unknown character '%c'(0x%02X)", _view(self, 0), _view(self, 0));
        is_float = true;
        break;
      default:
        _save_char(self, '\0');
        if (is_float) {
          candy_float_t f = (candy_float_t)strtod(_buffer(self), NULL);
          candy_wrap_init_float(wrap, &f, 1);
          return CANDY_TK_CST_FLOAT;
        }
        else {
          candy_integer_t i = (candy_integer_t)strtol(_buffer(self), NULL, 10);
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
  int head = self->io->w;
  const char del = _view(self, 0);
  /* skip first " or ' */
  _skip(self, multiline ? 3 : 1);
  while (_view(self, 0) != del) {
    switch (_view(self, 0)) {
      case '\0':
        lex_assert(false, "unexpected end of string");
        return -1;
      case '\r': case '\n':
        if (multiline == false) {
          lex_assert(false, "single line string can not contain line break");
          return -1;
        }
        _skip_newline(self);
        break;
      case '\\':
        _read(self);
        switch (_view(self, 0)) {
          case         'a': _read(self); _save_char(self,             '\a'); break;
          case         'b': _read(self); _save_char(self,             '\b'); break;
          case         't': _read(self); _save_char(self,             '\t'); break;
          case         'n': _read(self); _save_char(self,             '\n'); break;
          case         'v': _read(self); _save_char(self,             '\v'); break;
          case         'f': _read(self); _save_char(self,             '\f'); break;
          case         'r': _read(self); _save_char(self,             '\r'); break;
          case        '\\': _read(self); _save_char(self,             '\\'); break;
          case        '\'': _read(self); _save_char(self,             '\''); break;
          case         '"': _read(self); _save_char(self,              '"'); break;
          case         'x': _read(self); _save_char(self, _get_hexch(self)); break;
          case '0' ... '7':              _save_char(self, _get_octch(self)); break;
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
  _save_char(self, '\0');
  /* skip last " or ' */
  lex_assert(_view(self, 0) == del && (multiline ? (_view(self, 1) == del && _view(self, 2) == del) : (true)), "unexpected end of string");
  _skip(self, multiline ? 3 : 1);
  return self->io->w - head - 1;
}

static candy_tokens_t _get_ident_or_keyword(candy_lexer_t *self, candy_wrap_t *wrap) {
  /* save alpha, or '_' */
  _save(self);
  /* save number, alpha, or '_' */
  while (is_dec(_view(self, 0)) || is_alpha(_view(self, 0)) || _view(self, 0) == '_')
    _save(self);
  _save_char(self, '\0');
  /* check keyword */
  for (unsigned i = 0; i < candy_lengthof(_keywords); i++) {
    if (strcmp(_buffer(self), _keywords[i].keyword) == 0)
      return _keywords[i].token;
  }
  // wrap->hash = djb_hash(self->buffer->data);
  return CANDY_TK_IDENT;
}

static candy_tokens_t _lexer(candy_lexer_t *self, candy_wrap_t *wrap) {
  self->io->w = 0;
  while (1) {
    switch (_view(self, 0)) {
      case '\0':
        return CANDY_TK_NONE;
      case '\r': case '\n':
        _skip_newline(self);
        break;
      case ' ': case '\f': case '\t': case '\v':
        _read(self);
        break;
      case '!':
        lex_assert(_view(self, 1) == '=', "unexpected char '%c'", _view(self, 0));
        goto dual_ope;
      /* 'o', 'o=', 'oo' */
      case '>': case '<': case '*': case '/':
        /* 'oo' */
        if (_view(self, 1) == _view(self, 0))
          goto dual_ope;
      /* 'o', 'o=' */
      case '%': case '=': case '+': case '-':
        /* 'o=' */
        if (_view(self, 1) == '=')
          goto dual_ope;
      /* 'o' */
      case '&': case '|': case '~': case '^':
      case '(': case ')': case '[': case ']':
      case ',': case '.': case ':':
        return _read(self);
      /* is singleline comment */
      case '#':
        _skip_comment(self);
        break;
      /* is string */
      case '"': case '\'':
        candy_wrap_init_string(wrap, _buffer(self), _get_string(self, _view(self, 1) == _view(self, 0) && _view(self, 2) == _view(self, 0)));
        return CANDY_TK_CST_STRING;
      case '0' ... '9':
        return _get_number(self, wrap);
      default:
        if (is_alpha(_view(self, 0)) || _view(self, 0) == '_')
          return _get_ident_or_keyword(self, wrap);
        lex_assert(false, "unexpected char '%c'(0x%02X)", _view(self, 0), _view(self, 0));
        break;
    }
  }
  dual_ope:
  return tk_dual_ope(_read(self), _read(self));
}

int candy_lexer_init(candy_lexer_t *self, candy_io_t *io) {
  memset(self, 0, sizeof(struct candy_lexer));
  self->io = io;
#ifdef CANDY_DEBUG_MODE
  self->dbg.line = 1;
  self->dbg.column = 0;
#endif /* CANDY_DEBUG_MODE */
  self->lookahead.token = CANDY_TK_EOS;
  return 0;
}

int candy_lexer_deinit(candy_lexer_t *self) {
  candy_wrap_deinit(&self->lookahead.wrap);
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

void candy_lexer_assert(candy_lexer_t *self, const char format[], ...) {
  va_list ap;
  va_start(ap, format);
  vsnprintf(self->io->buffer, self->io->size, format, ap);
  va_end(ap);
  longjmp(self->io->rollback, 1);
}
