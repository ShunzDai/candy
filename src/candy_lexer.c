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

#define lex_assert(_condition, _format, ...) candy_assert(_condition, lexical, _format, ##__VA_ARGS__)

/**
  * @brief  observer mode of the stream
  * @param  self lexer
  * @param  idx  lookahead to the idx byte, idx value between 0 and @ref CANDY_LEXER_LOOKAHEAD_SIZE - 1
  * @retval target byte
  */
static inline char _view(candy_lexer_t *self, size_t idx) {
  assert(self->r + idx < self->io->size);
  return self->io->buff[self->r + idx];
}

/**
  * @brief  read out the next byte of the stream
  * @param  self lexer
  * @retval target byte
  */
static inline char _read(candy_lexer_t *self) {
  size_t size = self->io->size;
  /** only @ref CANDY_LEXER_LOOKAHEAD_SIZE bytes left to read */
  if (self->r + CANDY_LEXER_LOOKAHEAD_SIZE == size) {
    /* calculates the start position of the read buffer */
    size_t offset = self->w + CANDY_LEXER_EXTRA_SIZE + CANDY_LEXER_LOOKAHEAD_SIZE;
    /** if the number of bytes that can be filled is less than
        @ref CANDY_DEFAULT_IO_SIZE bytes, the buffer will be enlarged */
    if (size - offset < CANDY_DEFAULT_IO_SIZE) {
      candy_io_expand(self->io);
      self->reader(self->io->buff + size, self->io->size - size, self->ud);
    }
    /* otherwise buffer will be filled directly */
    else {
      /** temporarily stores @ref CANDY_LEXER_LOOKAHEAD_SIZE bytes that have not been read */
      char ahead[CANDY_LEXER_LOOKAHEAD_SIZE];
      memcpy(ahead, self->io->buff + self->r, CANDY_LEXER_LOOKAHEAD_SIZE);
      /* fill buffer */
      self->reader(self->io->buff + offset, size - offset, self->ud);
      /** restore the unread @ref CANDY_LEXER_LOOKAHEAD_SIZE bytes to the buffer */
      memcpy(self->io->buff + self->w + CANDY_LEXER_EXTRA_SIZE, ahead, CANDY_LEXER_LOOKAHEAD_SIZE);
      self->r = self->w + CANDY_LEXER_EXTRA_SIZE;
    }
  }
  #ifdef CANDY_DEBUG_MODE
  ++self->dbg.column;
  #endif /* CANDY_DEBUG_MODE */
  return self->io->buff[self->r++];
}

/**
  * @brief  skip the next byte of the stream
  * @param  self lexer
  * @retval none
  */
static inline void _skip(candy_lexer_t *self) {
  _read(self);
}

/**
  * @brief  skipping n bytes of the stream
  * @param  self lexer
  * @param  n    count of bytes
  * @retval none
  */
static inline void _skipn(candy_lexer_t *self, int n) {
  while (n--)
    _skip(self);
}

/**
  * @brief  store a byte
  * @param  self lexer
  * @param  ch   byte to be stored
  * @retval none
  */
static inline void _save_char(candy_lexer_t *self, char ch) {
  assert(self->r - self->w >= CANDY_LEXER_EXTRA_SIZE);
  self->io->buff[self->w++] = ch;
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
  pred(self);
  _check_dual(self, "\r\n", pred);
#ifdef CANDY_DEBUG_MODE
  self->dbg.line++;
  self->dbg.column = 0;
#endif /* CANDY_DEBUG_MODE */
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
      case (char)EOF:
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
static inline void _save_oct(candy_lexer_t *self) {
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
static inline void _save_hex(candy_lexer_t *self) {
  lex_assert(is_hex(_view(self, 0)) && is_hex(_view(self, 1)), "invalid hexadecimal escape");
  _save_char(self, ch2hex(_read(self)) << 4 | ch2hex(_read(self)));
}

static candy_tokens_t _get_number(candy_lexer_t *self, candy_wrap_t *wrap) {
  bool is_float = false;
  if (((_view(self, 0) == '0') ? _save(self), true : false) && _check_dual(self, "xX", _save)) {
    while (_check_next(self, is_hex, _save));
    _save_char(self, '\0');
    candy_integer_t i = (candy_integer_t)strtol(self->io->buff, NULL, 16);
    candy_wrap_set_integer(wrap, &i, 1);
    return TK_INTEGER;
  }
  _save(self);
  while (1) {
    switch (_view(self, 0)) {
      case '.':
        lex_assert(!is_float, "multiple dots appear");
        _save(self);
        lex_assert(is_dec(_view(self, 0)), "unknown character '%c'(0x%02X)", _view(self, 0), _view(self, 0));
        is_float = true;
        break;
      case 'e': case 'E':
        _save(self);
        _check_dual(self, "+-", _save);
        lex_assert(is_dec(_view(self, 0)), "unknown character '%c'(0x%02X)", _view(self, 0), _view(self, 0));
        is_float = true;
        break;
      default:
        if (_check_next(self, is_dec, _save))
          break;
        _save_char(self, '\0');
        if (is_float) {
          candy_float_t f = (candy_float_t)strtod(self->io->buff, NULL);
          candy_wrap_set_float(wrap, &f, 1);
          return TK_FLOAT;
        }
        else {
          candy_integer_t i = (candy_integer_t)strtol(self->io->buff, NULL, 10);
          candy_wrap_set_integer(wrap, &i, 1);
          return TK_INTEGER;
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
  * @retval tokens enum
  */
static candy_tokens_t _get_string(candy_lexer_t *self, candy_wrap_t *wrap, const bool multiline) {
  const char del = _view(self, 0);
  /* skip first " or ' */
  _skipn(self, multiline ? 3 : 1);
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
        _save(self);
        break;
    }
  }
  /* skip last " or ' */
  lex_assert(multiline ? (_view(self, 1) == del && _view(self, 2) == del) : true, "unexpected end of string");
  _skipn(self, multiline ? 3 : 1);
  candy_wrap_set_string(wrap, self->io->buff, self->w);
  return TK_STRING;
}

static candy_tokens_t _get_ident_or_keyword(candy_lexer_t *self, candy_wrap_t *wrap) {
  static const struct {
    candy_tokens_t token;
    const char * const keyword;
  } _keywords[] = {
    #define CANDY_KW_MATCH
    #include "src/candy_keyword.list"
  };
  /* save alpha, or '_' */
  _save(self);
  /* save number, alpha, or '_' */
  while (is_dec(_view(self, 0)) || is_alpha(_view(self, 0)) || _view(self, 0) == '_')
    _save(self);
  /* check keyword */
  for (size_t i = 0; i < candy_lengthof(_keywords); i++) {
    if (strncmp(self->io->buff, _keywords[i].keyword, self->w) == 0)
      return _keywords[i].token;
  }
  candy_wrap_set_string(wrap, self->io->buff, self->w);
  return TK_IDENT;
}

static candy_tokens_t _lexer(candy_lexer_t *self, candy_wrap_t *wrap) {
  self->w = 0;
  while (1) {
    switch (_view(self, 0)) {
      case (char)EOF:
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
      /* 'o', 'o=' */
      case '%': case '=': case '+': case '-':
        /* 'o=' */
        if (_view(self, 1) == '=')
          goto binary;
      /* 'o' */
      case '&': case '|': case '~': case '^':
      case '(': case ')': case '[': case ']':
      case ',': case ':':
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
        else if (is_alpha(_view(self, 0)) || _view(self, 0) == '_')
          return _get_ident_or_keyword(self, wrap);
        lex_assert(false, "unknown character '%c'(0x%02X)", _view(self, 0), _view(self, 0));
        return -1;
    }
  }
  /* double-byte operator */
  binary:
  return binary_ope(_read(self), _read(self));
}

int candy_lexer_init(candy_lexer_t *self, candy_io_t *io, candy_reader_t reader, void *ud) {
  memset(self, 0, sizeof(struct candy_lexer));
#ifdef CANDY_DEBUG_MODE
  self->dbg.line = 1;
  self->dbg.column = 0;
#endif /* CANDY_DEBUG_MODE */
  self->lookahead.token = TK_EOS;
  self->io = io;
  self->reader = reader;
  self->ud = ud;
  self->r = CANDY_LEXER_EXTRA_SIZE;
  self->w = 0;
  self->reader(self->io->buff + CANDY_LEXER_EXTRA_SIZE, self->io->size - CANDY_LEXER_EXTRA_SIZE, self->ud);
  return 0;
}

int candy_lexer_deinit(candy_lexer_t *self) {
  candy_wrap_deinit(&self->lookahead.wrap);
  return 0;
}

candy_tokens_t candy_lexer_next(candy_lexer_t *self, candy_wrap_t *wrap) {
  /* is there a look-ahead token? */
  if (self->lookahead.token != TK_EOS) {
    /* use this one */
    candy_tokens_t token = self->lookahead.token;
    if (wrap)
      *wrap = self->lookahead.wrap;
    /* and discharge it */
    self->lookahead.token = TK_EOS;
    return token;
  }
  /* read next token */
  return _lexer(self, wrap);
}

candy_tokens_t candy_lexer_lookahead(candy_lexer_t *self) {
  if (self->lookahead.token == TK_EOS)
    self->lookahead.token = _lexer(self, &self->lookahead.wrap);
  return self->lookahead.token;
}
