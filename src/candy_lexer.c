/**
  * Copyright 2022-2023 ShunzDai
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
#include "src/candy_lib.h"
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#define lex_assert(_condition, _format, ...) candy_assert(*(candy_io_t **)(self), _condition, lexical, _format, ##__VA_ARGS__)

static char *_buff(candy_lexer_t *self) {
  return (char *)candy_wrap_get_string(&self->io->buff);
}

static size_t _size(candy_lexer_t *self) {
  return candy_wrap_size(&self->io->buff);
}

static int _fill(candy_lexer_t *self, size_t ahead) {
  size_t size = _size(self);
  if (self->r + ahead < size)
    return 0;
  /* calculate the start position of the read buffer */
  size_t offset = self->w + ahead;
  /** if the number of bytes that can be filled is less than
      @ref CANDY_LEXER_EXPAND_SIZE bytes, the buffer will be enlarged */
  if (size < CANDY_LEXER_EXPAND_SIZE + offset) {
    candy_wrap_append(&self->io->buff, NULL, CANDY_LEXER_EXPAND_SIZE);
    offset = size;
  }
  /* otherwise buffer will be filled directly */
  else {
    /* restore the unread remain bytes to the buffer */
    memmove(_buff(self) + self->w, _buff(self) + self->r, ahead);
    self->r = self->w;
  }
  /* fill buffer */
  int res = self->reader(_buff(self) + offset, _size(self) - offset, self->ud);
  lex_assert(res > 0, "stream error");
  return res;
}

/**
  * @brief  observer mode of the stream
  * @param  self lexer
  * @param  idx  lookahead to the idx byte, idx value between 0 and @ref CANDY_LEXER_LOOKAHEAD_SIZE - 1
  * @retval target byte
  */
static char _view(candy_lexer_t *self, int idx) {
  _fill(self, idx + 1);
  assert(self->r + idx < _size(self));
  return _buff(self)[self->r + idx];
}

/**
  * @brief  read out the next byte of the stream
  * @param  self lexer
  * @retval target byte
  */
static char _read(candy_lexer_t *self) {
  _fill(self, 1);
  ++self->dbg.column;
  return _buff(self)[self->r++];
}

/**
  * @brief  skip the next byte of the stream
  * @param  self lexer
  * @retval none
  */
static void _skip(candy_lexer_t *self) {
  _read(self);
}

/**
  * @brief  skipping n bytes of the stream
  * @param  self lexer
  * @param  n    count of bytes
  * @retval none
  */
static void _skipn(candy_lexer_t *self, int n) {
  while (n--)
    _skip(self);
}

/**
  * @brief  store a byte
  * @param  self lexer
  * @param  ch   byte to be stored
  * @retval none
  */
static void _save_char(candy_lexer_t *self, char ch) {
  assert(self->r > self->w);
  _buff(self)[self->w++] = ch;
}

/**
  * @brief  store the next byte of the stream
  * @param  self lexer
  * @retval none
  */
static void _save(candy_lexer_t *self) {
  _save_char(self, _read(self));
}

/**
  * @brief  check whether the next byte of the stream satisfies the check function,
  *         if so, process this byte with the predicate
  * @param  self  lexer
  * @param  check check function
  * @param  pred  predicate
  * @retval boolean
  */
static inline bool _check_next(candy_lexer_t *self, bool (*check)(char), void (*pred)(candy_lexer_t *)) {
  return check(_view(self, 0)) ? pred(self), true : false;
}

/**
  * @brief  checks whether the next byte of the stream is one of the two bytes the user expects,
  *         if so, process this byte with the predicate
  * @param  self lexer
  * @param  str  the string that the user expects
  * @param  pred predicate
  * @retval boolean
  */
static inline bool _check_dual(candy_lexer_t *self, const char str[], void (*pred)(candy_lexer_t *)) {
  return (_view(self, 0) == str[0] || _view(self, 0) == str[1]) ? pred(self), true : false;
}

/**
  * @brief  get newline, like
  *         \r
  *         \n
  *         \r\n
  *         \n\r
  *         first byte has been checked by up layer function
  * @param  self lexer
  * @param  pred predicate
  * @return none
  */
static void _handle_newline(candy_lexer_t *self, void (*pred)(candy_lexer_t *)) {
  char old = _view(self, 0);
  pred(self);
  if (_view(self, 0) == "\r\n"[old == '\r'])
    pred(self);
  self->dbg.line++;
  self->dbg.column = 1;
}

/**
  * @brief  skip comment, like
  *         # hello world
  * @param  self lexer
  * @retval none
  */
static void _skip_comment(candy_lexer_t *self) {
  _skip(self);
  while (1) {
    switch (_view(self, 0)) {
      case '\r': case '\n':
        _handle_newline(self, _skip);
      case '\0':
        return;
      default:
        _skip(self);
        break;
    }
  }
}

/**
  * @brief  save octal escape character, like
  *         "0"
  *         "01"
  *         "012"
  * @param  self lexer
  * @retval none
  */
static void _save_oct(candy_lexer_t *self) {
  char buff[] = {_view(self, 0), _view(self, 1), _view(self, 2), '\0'};
  char *end = NULL;
  char value = strtoul(buff, &end, 8);
  _skipn(self, end - buff);
  _save_char(self, value);
}

/**
  * @brief  save hexadecimal escape character, like
  *         "30"
  * @param  self lexer
  * @retval none
  */
static void _save_hex(candy_lexer_t *self) {
  lex_assert(is_hex(_view(self, 0)) && is_hex(_view(self, 1)), "invalid hexadecimal escape");
  _save_char(self, chtonum(_read(self)) << 4 | chtonum(_read(self)));
}

static candy_tokens_t _get_number(candy_lexer_t *self, candy_wrap_t *wrap) {
  candy_tokens_t token = TK_INTEGER;
  bool(*check)(char) = is_dec;
  char first = _read(self);
  _save_char(self, first);
  if (first == '0') {
    if (_check_dual(self, "Xx", _skip)) {
      lex_assert(is_hex(_view(self, 0)), "invalid hexadecimal number");
      check = is_hex;
    }
    else if (_check_dual(self, "Bb", _skip)) {
      lex_assert(is_bin(_view(self, 0)), "invalid binary number");
      check = is_bin;
    }
  }
  while (1) {
    if (check(_view(self, 0)))
      _save(self);
    else if (_check_dual(self, "Ee", _save)) {
      lex_assert(check == is_dec, "invalid float number");
      _check_dual(self, "+-", _save);
      token = TK_FLOAT;
    }
    else if (_view(self, 0) == '.') {
      lex_assert(check == is_dec, "invalid float number");
      _save(self);
      token = TK_FLOAT;
    }
    else {
      lex_assert(!is_alpha(_view(self, 0)), "extra text after expected end of number");
      break;
    }
  }
  char *end = NULL;
  if (token == TK_INTEGER) {
    int base = check == is_dec ? first == '0' ? 8 : 10 : check == is_hex ? 16 : 2;
    candy_integer_t i = (candy_integer_t)strntol(_buff(self), self->w, &end, base);
    candy_wrap_set_integer(wrap, &i, 1);
  }
  else {
    candy_float_t f = (candy_float_t)strntod(_buff(self), self->w, &end);
    candy_wrap_set_float(wrap, &f, 1);
  }
  lex_assert(_buff(self) + self->w == end, "malformed number");
  return token;
}

/**
  * @brief  gets token of type string, like
  *         "hello world\n"
  *         'A\tB\tC'
  *         "\x41\x42\x43"
  *         '\041\042\043'
  * @param  self lexer
  * @param  multiline is multiline string or not
  * @retval tokens enum
  */
static candy_tokens_t _get_string(candy_lexer_t *self, candy_wrap_t *wrap, const bool multiline) {
  const char del = _view(self, 0);
  /* skip first " or ' */
  _skipn(self, multiline ? 3 : 1);
  while (1) {
    switch (_view(self, 0)) {
      case '\0':
        lex_assert(false, "unexpected end of string");
        return -1;
      case '\r': case '\n':
        lex_assert(multiline, "unexpected end of string");
        _handle_newline(self, _save);
        break;
      case '\\':
        _skip(self);
        switch (_view(self, 0)) {
          case  'a': _skip(self); _save_char(self,      '\a'); break;
          case  'b': _skip(self); _save_char(self,      '\b'); break;
          case  't': _skip(self); _save_char(self,      '\t'); break;
          case  'n': _skip(self); _save_char(self,      '\n'); break;
          case  'v': _skip(self); _save_char(self,      '\v'); break;
          case  'f': _skip(self); _save_char(self,      '\f'); break;
          case  'r': _skip(self); _save_char(self,      '\r'); break;
          case '\\': _skip(self); _save_char(self,      '\\'); break;
          case '\'': _skip(self); _save_char(self,      '\''); break;
          case  '"': _skip(self); _save_char(self,       '"'); break;
          case  'x': _skip(self); _save_hex(self);             break;
          case  'u': lex_assert(false, "unsupported unicode"); break;
          default:
            if (_check_next(self, is_oct, _save_oct))
              break;
            /* is not escape */
            _save_char(self, '\\');
            _save(self);
            break;
        }
        break;
      /* normal character */
      default:
        if (_view(self, 0) == del && (!multiline || (_view(self, 1) == del && _view(self, 2) == del)))
          goto exit;
        _save(self);
        break;
    }
  }
  exit:
  _skipn(self, multiline ? 3 : 1);
  candy_wrap_set_string(wrap, _buff(self), self->w);
  return TK_STRING;
}

static candy_tokens_t _get_ident_or_keyword(candy_lexer_t *self, candy_wrap_t *wrap) {
  /* save alpha */
  _save(self);
  /* save alpha or number */
  while (is_alnum(_view(self, 0)))
    _save(self);
  /* check keyword */
  switch (djb_hash(_buff(self), self->w)) {
    #define CANDY_KW_MATCH
    #include "src/candy_keyword.list"
    default:
      candy_wrap_set_string(wrap, _buff(self), self->w);
      return TK_IDENT;
  }
}

static candy_tokens_t _lexer(candy_lexer_t *self, candy_wrap_t *wrap) {
  self->w = 0;
  self->lookahead.wrap = CANDY_WRAP_NULL;
  while (1) {
    switch (_view(self, 0)) {
      case '\0':
        return TK_EOS;
      case '\r': case '\n':
        _handle_newline(self, _skip);
        break;
      case ' ': case '\f': case '\t': case '\v':
        _skip(self);
        break;
      case '!':
        lex_assert(_view(self, 1) == '=', "unknown character '%c'(0x%02X)", _view(self, 1), _view(self, 1));
        goto binary;
      /* 'o', 'o=', 'oo' */
      case '>': case '<': case '*': case '/':
        /* 'oo' */
        if (_view(self, 1) == _view(self, 0))
          goto binary;
        /* fall through */
      /* 'o', 'o=' */
      case '%': case '=': case '+': case '-':
        /* 'o=' */
        if (_view(self, 1) == '=')
          goto binary;
        /* fall through */
      /* 'o' */
      case '&': case '|': case '~': case '^': case ',': case ':':
      case '(': case ')': case '[': case ']': case '{': case '}':
      case ';': case '@':
        return _read(self);
      case '.':
        return (_view(self, 1) == _view(self, 0) && _view(self, 2) == _view(self, 0)) ? _skipn(self, 3), TK_VARARG : _read(self);
      /* is comment */
      case '#':
        _skip_comment(self);
        break;
      /* is string */
      case '"': case '\'':
        return _get_string(self, wrap, _view(self, 1) == _view(self, 0) && _view(self, 2) == _view(self, 0));
      default:
        if (is_dec(_view(self, 0)))
          return _get_number(self, wrap);
        else if (is_alpha(_view(self, 0)))
          return _get_ident_or_keyword(self, wrap);
        lex_assert(false, "unrecognized token");
    }
  }
  /* double-byte operator */
  binary:
  return binary_ope(_read(self), _read(self));
}

int candy_lexer_init(candy_lexer_t *self, candy_io_t *io, candy_reader_t reader, void *ud) {
  memset(self, 0, sizeof(struct candy_lexer));
  self->dbg.line = 1;
  self->dbg.column = 1;
  self->lookahead.token = TK_EOS;
  self->io = io;
  self->reader = reader;
  self->ud = ud;
  self->w = 0;
  self->r = self->w;
  return 0;
}

int candy_lexer_deinit(candy_lexer_t *self) {
  return 0;
}

candy_tokens_t candy_lexer_lookahead(candy_lexer_t *self) {
  if (self->lookahead.token == TK_EOS)
    self->lookahead.token = _lexer(self, &self->lookahead.wrap);
  return self->lookahead.token;
}

const candy_wrap_t *candy_lexer_next(candy_lexer_t *self) {
  self->lookahead.token = TK_EOS;
  return &self->lookahead.wrap;
}
