/**
  * Copyright 2022-2025 ShunzDai
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
#include "core/candy_lexer.h"
#include "core/candy_lib.h"
#include "core/candy_exception.h"
#include "core/candy_gc.h"
#include "core/candy_array.h"
#include <string.h>

#define lex_assert(_condition, _format, ...) \
candy_assert(self->ctx, self->gc, _condition, CANDY_ERR_LEXICAL, \
_format, ##__VA_ARGS__)

static const char TAG[] = "lexer";

static inline const char *_head(candy_lexer_t *self) {
  return candy_buffer_head(&self->buff);
}

static inline size_t _size(candy_lexer_t *self) {
  return candy_buffer_size(&self->buff);
}

static inline void _reset(candy_lexer_t *self) {
  candy_buffer_reset(&self->buff);
}

static inline char _view(candy_lexer_t *self, size_t ahead) {
  char ch = 0;
  int res = candy_buffer_view(&self->buff, candy_gc_memory(self->gc), self->ctx, &ch, ahead);
  lex_assert(res >= 0, "abnormal input stream");
  return ch;
}

static inline void _readn(candy_lexer_t *self, char str[], size_t size) {
  int res = candy_buffer_read(&self->buff, candy_gc_memory(self->gc), self->ctx, str, size);
  lex_assert(res >= 0, "abnormal input stream");
  self->dbg.column += size;
}

static inline char _read(candy_lexer_t *self) {
  char ch = 0;
  _readn(self, &ch, 1);
  return ch;
}

static inline void _skipn(candy_lexer_t *self, size_t n) {
  _readn(self, NULL, n);
}

static inline void _skip(candy_lexer_t *self) {
  _skipn(self, 1);
}

static inline void _save_char(candy_lexer_t *self, char ch) {
  candy_buffer_write(&self->buff, &ch, 1);
}

static inline void _save(candy_lexer_t *self) {
  _save_char(self, _read(self));
}

/**
   * @brief  Check whether the next byte of the stream satisfies the check function.
   * If so, it applies the predicate to this byte.
 * @param  self  lexer
 * @param  check check function
 * @param  pred  predicate
 * @retval boolean
 */
static inline bool _check_next(candy_lexer_t *self, bool (*check)(char), void (*pred)(candy_lexer_t *)) {
  return check(_view(self, 0)) ? pred(self), true : false;
}

/**
 * @brief  Checks whether the next byte of the stream is one of the two bytes it expects.
 * If so, it applies the predicate to this byte.
 * @param  self lexer
 * @param  str  the string that the expects
 * @param  pred predicate
 * @retval boolean
 */
static inline bool _check_dual(candy_lexer_t *self, const char str[], void (*pred)(candy_lexer_t *)) {
  return (_view(self, 0) == str[0] || _view(self, 0) == str[1]) ? pred(self), true : false;
}

/**
 * @brief  Get newline, like '\r', '\n', '\r\n', '\n\r'.
 * First byte has been checked by up layer function.
 * @param  self lexer
 * @param  pred predicate
 * @return none
 */
static void _handle_newline(candy_lexer_t *self, void (*pred)(candy_lexer_t *)) {
  char ch = _view(self, 0);
  pred(self);
  if (_view(self, 0) == "\r\n"[ch == '\r'])
    pred(self);
  self->dbg.line++;
  self->dbg.column = 1;
}

static void _skip_line(candy_lexer_t *self) {
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
 * @brief  Save octal escape character, like "0", "01", "012".
 * @param  self lexer
 * @retval none
 */
static void _save_oct(candy_lexer_t *self) {
  char buff[] = {_view(self, 0), _view(self, 1), _view(self, 2)};
  char *end = NULL;
  char value = strntol(buff, sizeof(buff), &end, 8);
  _skipn(self, end - buff);
  _save_char(self, value);
}

/**
 * @brief  Save hexadecimal escape character, like "30".
 * @param  self lexer
 * @retval none
 */
static void _save_hex(candy_lexer_t *self) {
  lex_assert(is_hex(_view(self, 0)) && is_hex(_view(self, 1)), "invalid hexadecimal escape");
  _save_char(self, chtonum(_read(self)) << 4 | chtonum(_read(self)));
}

static candy_tokens_t _get_number(candy_lexer_t *self, candy_meta_t *meta) {
  char first = _view(self, 0);
  candy_tokens_t token = TK_INTEGER;
  bool(*check)(char) = is_dec;
  _save(self);
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
    else if (!is_alpha(_view(self, 0))) {
      break;
    }
    else {
      lex_assert(false, "extra text after expected end of number");
    }
  }
  char *end = NULL;
  if (token == TK_INTEGER) {
    int base = check == is_dec ? first == '0' ? 8 : 10 : check == is_hex ? 16 : 2;
    meta->i = (candy_integer_t)strntol(_head(self), _size(self), &end, base);
  }
  else {
    meta->f = (candy_float_t)strntod(_head(self), _size(self), &end);
  }
  lex_assert(_head(self) + _size(self) == end, "malformed number");
  return token;
}

/**
 * @brief  gets token of type string, like "hello world\n", 'A\tB\tC',
 * "\x41\x42\x43", '\041\042\043'.
 * @param  self lexer
 * @param  meta container
 * @retval tokens enum
 */
static candy_tokens_t _get_string(candy_lexer_t *self, candy_meta_t *meta) {
  const char del = _view(self, 0);
  const bool multiline = _view(self, 1) == del && _view(self, 2) == del;
  /* skip " or ' */
  _skipn(self, multiline ? 3 : 1);
  while (1) {
    switch (_view(self, 0)) {
      case '\0':
        lex_assert(false, "unexpected end of string");
        return TK_ERR;
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
            /* is octal escape */
            if (_check_next(self, is_oct, _save_oct))
              break;
            /* is line-continuation */
            if (_check_dual(self, "\r\n", _skip_line))
              break;
            /* neither of them */
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
  candy_logd(TAG, "string <%.*s>", (int)_size(self), _head(self));
  meta->s = candy_array_create_const(self->gc, self->ctx, CANDY_TYPE_CHAR, _head(self), _size(self));
  return TK_STRING;
}

static candy_tokens_t _get_ident_or_keyword(candy_lexer_t *self, candy_meta_t *meta) {
  /* save alpha */
  _save(self);
  /* save alpha or number */
  while (_check_next(self, is_alnum, _save));
  /* check keyword */
  switch (hash_djb(_head(self), _size(self))) {
    #define CANDY_KW_MATCH
    #include "core/candy_keyword.list"
    default:
      candy_logd(TAG, "ident <%.*s>", (int)_size(self), _head(self));
      meta->s = candy_array_create_const(self->gc, self->ctx, CANDY_TYPE_CHAR, _head(self), _size(self));
      return TK_IDENT;
  }
}

static candy_tokens_t _lexer(candy_lexer_t *self, candy_meta_t *meta) {
  _reset(self);
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
        goto op2;
      /* 'o', 'o=', 'oo' */
      case '>': case '<':
        /* 'oo' */
        if (_view(self, 1) == _view(self, 0))
          goto op2;
        /* fall through */
      /* 'o', 'o=' */
      case '=': case '&': case '|': case '^':
      case '+': case '-': case '*': case '/': case '%':
        /* 'o=' */
        if (_view(self, 1) == '=')
          goto op2;
        /* fall through */
      /* 'o' */
      case '~': case ',': case ':': case ';': case '@':
      case '(': case ')': case '[': case ']': case '{': case '}':
        goto op1;
      case '.':
        if (_view(self, 1) == '.' && _view(self, 2) == '.')
          goto op3;
        goto op1;
      /* is comment */
      case '#':
        _skip(self);
        _skip_line(self);
        break;
      /* is string */
      case '"': case '\'':
        return _get_string(self, meta);
      /* is line-continuation */
      case '\\':
        _skip(self);
        if (_check_dual(self, "\r\n", _skip_line))
          break;
        lex_assert(false, "unexpected character after line continuation character");
      default:
        if (is_dec(_view(self, 0)))
          return _get_number(self, meta);
        else if (is_alpha(_view(self, 0)))
          return _get_ident_or_keyword(self, meta);
        lex_assert(false, "unrecognized token");
    }
  }
  op1:
  return gen_operator(_read(self));
  op2:
  return gen_operator(_read(self), _read(self));
  op3:
  return gen_operator(_read(self), _read(self), _read(self));
}

candy_err_t candy_lexer_init(candy_lexer_t *self, candy_gc_t *gc, candy_excep_t *ctx, candy_reader_t reader, void *arg) {
  memset(self, 0, sizeof(candy_lexer_t));
  candy_buffer_init(&self->buff, reader, arg);
  self->dbg.line = 1;
  self->dbg.column = 1;
  self->lookahead.token = TK_EOS;
  self->ctx = ctx;
  self->gc = gc;
  return CANDY_OK;
}

candy_err_t candy_lexer_deinit(candy_lexer_t *self) {
  candy_buffer_deinit(&self->buff, candy_gc_memory(self->gc));
  return CANDY_OK;
}

candy_tokens_t candy_lexer_lookahead(candy_lexer_t *self) {
  if (self->lookahead.token == TK_EOS) {
    self->lookahead.meta = (candy_meta_t){};
    self->lookahead.token = _lexer(self, &self->lookahead.meta);
  }
  candy_logd(TAG, "lookahead %s", candy_token_str(self->lookahead.token));
  return self->lookahead.token;
}

const candy_meta_t *candy_lexer_next(candy_lexer_t *self) {
  lex_assert(self->lookahead.token != TK_EOS, "not lookahead yet");
  self->lookahead.token = TK_EOS;
  return &self->lookahead.meta;
}
