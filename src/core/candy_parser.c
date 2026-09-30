/**
  * Copyright 2022-2026 ShunzDai
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
#include "core/candy_parser.h"
#include "core/candy_exception.h"
#include "core/candy_wrap.h"
#include "core/candy_gc.h"
#include "core/candy_proto.h"
#include "core/candy_closure.h"
#include "core/candy_array.h"
#include "core/candy_lexer.h"

#define par_assert(_condition, _format, ...) \
candy_assert(self->ls.ctx, self->ls.gc, _condition, CANDY_ERR_SYNTAX, \
  "line %zu col %zu: " _format, self->ls.dbg.line, self->ls.dbg.column, ##__VA_ARGS__)

typedef struct funcstate funcstate_t;
typedef struct parser parser_t;

struct funcstate {
  funcstate_t *prev;
  candy_proto_t *proto;
  const candy_array_t *name;
  const candy_array_t *locals[UINT8_MAX];
  uint8_t nlocal;
};

struct parser {
  candy_lexer_t ls;
  funcstate_t *fs;
};

static void _funcstate_open(funcstate_t *self, parser_t *prsr) {
  self->prev = prsr->fs;
  self->proto = candy_proto_create(prsr->ls.gc, prsr->ls.ctx);
  self->name = NULL;
  self->nlocal = 0;
  prsr->fs = self;
}

static void _funcstate_close(funcstate_t *self, parser_t *prsr) {
  prsr->fs = self->prev;
}

static size_t _add_const(parser_t *self, candy_wrap_t *wrap) {
  return candy_proto_add_const(self->fs->proto, self->ls.gc, self->ls.ctx, wrap);
}

static size_t _add_name_const(parser_t *self, const candy_array_t *name) {
  candy_wrap_t wrap;
  candy_wrap_set_object(&wrap, (const candy_object_t *)name);
  return _add_const(self, &wrap);
}

static size_t _add_meta(parser_t *self, candy_tokens_t token) {
  candy_wrap_t wrap;
  switch (token) {
    case TK_INTEGER: candy_wrap_set_integer(&wrap, candy_lexer_next(&self->ls)->i); break;
    case TK_FLOAT:   candy_wrap_set_float(&wrap, candy_lexer_next(&self->ls)->f); break;
    case TK_STRING:  candy_wrap_set_object(&wrap, (const candy_object_t *)candy_lexer_next(&self->ls)->s); break;
    case TK_true:    candy_lexer_next(&self->ls); candy_wrap_set_boolean(&wrap, true); break;
    case TK_false:   candy_lexer_next(&self->ls); candy_wrap_set_boolean(&wrap, false); break;
    case TK_none:
      candy_lexer_next(&self->ls);
      candy_wrap_set_type(&wrap, CANDY_TYPE_NONE);
      candy_wrap_set_mask(&wrap, MASK_CONST);
      break;
    default:
      par_assert(false, "unknown expression %s", candy_token_str(token));
      return 0;
  }
  return _add_const(self, &wrap);
}

static size_t _add_none(parser_t *self) {
  candy_wrap_t wrap;
  candy_wrap_set_type(&wrap, CANDY_TYPE_NONE);
  candy_wrap_set_mask(&wrap, MASK_CONST);
  return _add_const(self, &wrap);
}

static size_t _add_iax(parser_t *self, candy_opcodes_t op, uint32_t a) {
  if ((op == OP_SUBC || op == OP_LTC) && a < (1U << 18)) {
    const candy_vector_t *inst = candy_proto_get_inst(self->fs->proto);
    size_t count = candy_vector_size(inst);
    if (count > 0) {
      const candy_inst_t *previous = (const candy_inst_t *)candy_vector_data(inst) + count - 1;
      if (previous->op == OP_LOADL && previous->iax.a <= UINT8_MAX) {
        candy_proto_set_inst(self->fs->proto, count - 1, (candy_inst_t) {
          .iabx = {.op = op == OP_SUBC ? OP_SUBLC : OP_LTLC, .a = previous->iax.a, .b = a},
        });
        return count - 1;
      }
    }
  }
  return candy_proto_add_inst(self->fs->proto, self->ls.gc, self->ls.ctx, (candy_inst_t) {
    .iax = {.op = (uint32_t)op, .a = a},
  });
}

static size_t _add_iabx(parser_t *self, candy_opcodes_t op, uint32_t a, uint32_t b) {
  return candy_proto_add_inst(self->fs->proto, self->ls.gc, self->ls.ctx, (candy_inst_t) {
    .iabx = {.op = (uint32_t)op, .a = a, .b = b},
  });
}

static int _local(parser_t *self, const candy_array_t *name) {
  if (self->fs->name == name)
    return 0;
  for (uint8_t i = 0; i < self->fs->nlocal; ++i)
    if (self->fs->locals[i] == name)
      return (int)i + 1;
  return -1;
}

static void _check(parser_t *self, candy_tokens_t token) {
  par_assert(candy_lexer_lookahead(&self->ls) == token, "unexpected token %s, expected %s",
    candy_token_str(candy_lexer_lookahead(&self->ls)), candy_token_str(token));
}

static const candy_meta_t *_next(parser_t *self, candy_tokens_t token) {
  _check(self, token);
  return candy_lexer_next(&self->ls);
}

static bool _test(parser_t *self, candy_tokens_t token) {
  if (candy_lexer_lookahead(&self->ls) == token) {
    candy_lexer_next(&self->ls);
    return true;
  }
  return false;
}

static int _precedence(candy_tokens_t token) {
  switch (token) {
    case TK_LESS: case TK_GREATER: case TK_LEQUAL: case TK_GEQUAL: case TK_EQUAL: case TK_NEQUAL: return 10;
    case TK_PLUS: case TK_MINUS: return 20;
    case TK_ASTE: case TK_SLASH: case TK_PERCENT: return 30;
    default: return -1;
  }
}

static candy_err_t _expr(parser_t *self, int min_prec);

static void _name_expr(parser_t *self, const candy_array_t *name) {
  int local = _local(self, name);
  bool call = _test(self, '(');
  if (!call || local != 0) {
    if (local >= 0)
      _add_iax(self, OP_LOADL, (uint32_t)local);
    else
      _add_iax(self, OP_LOADG, (uint32_t)_add_name_const(self, name));
  }
  if (call) {
    uint32_t narg = 0;
    if (!_test(self, ')')) {
      do {
        _expr(self, 0);
        ++narg;
      } while (_test(self, ','));
      _next(self, ')');
    }
    _add_iabx(self, local == 0 ? OP_CALLSELF : OP_CALL, narg, 1);
  }
}

static candy_err_t _primary(parser_t *self) {
  candy_tokens_t token = candy_lexer_lookahead(&self->ls);
  if (token == '(') {
    candy_lexer_next(&self->ls);
    _expr(self, 0);
    _next(self, ')');
    return CANDY_OK;
  }
  if (token == TK_IDENT) {
    const candy_array_t *name = _next(self, TK_IDENT)->s;
    _name_expr(self, name);
    return CANDY_OK;
  }
  par_assert(token == TK_INTEGER || token == TK_FLOAT || token == TK_STRING ||
    token == TK_true || token == TK_false || token == TK_none,
    "unexpected token %s in expression", candy_token_str(token));
  _add_iax(self, OP_LOADC, (uint32_t)_add_meta(self, token));
  return CANDY_OK;
}

static candy_err_t _expr(parser_t *self, int min_prec) {
  _primary(self);
  while (1) {
    candy_tokens_t token = candy_lexer_lookahead(&self->ls);
    int prec = _precedence(token);
    if (prec < min_prec)
      break;
    candy_lexer_next(&self->ls);
    if ((token == TK_MINUS || token == TK_LESS) && candy_lexer_lookahead(&self->ls) == TK_INTEGER) {
      size_t constant = _add_meta(self, TK_INTEGER);
      _add_iax(self, token == TK_MINUS ? OP_SUBC : OP_LTC, (uint32_t)constant);
      continue;
    }
    _expr(self, prec + 1);
    switch (token) {
      case TK_PLUS:  _add_iax(self, OP_ADD, 0); break;
      case TK_MINUS: _add_iax(self, OP_SUB, 0); break;
      case TK_LESS:  _add_iax(self, OP_LT, 0); break;
      default:       par_assert(false, "operator %s is not supported", candy_token_str(token));
    }
  }
  return CANDY_OK;
}

static candy_err_t _block(parser_t *self, bool stop_at_end) {
  while (1) {
    candy_tokens_t token = candy_lexer_lookahead(&self->ls);
    if (token == TK_EOS || (stop_at_end && token == TK_end))
      break;
    switch (token) {
      case TK_def: {
        candy_lexer_next(&self->ls);
        const candy_array_t *name = _next(self, TK_IDENT)->s;
        funcstate_t fs;
        _funcstate_open(&fs, self);
        fs.name = name;
        _next(self, '(');
        if (!_test(self, ')')) {
          do {
            par_assert(self->fs->nlocal < UINT8_MAX, "too many parameters");
            self->fs->locals[self->fs->nlocal++] = _next(self, TK_IDENT)->s;
          } while (_test(self, ','));
          _next(self, ')');
        }
        _block(self, true);
        _next(self, TK_end);
        size_t none = _add_none(self);
        _add_iax(self, OP_LOADC, (uint32_t)none);
        _add_iax(self, OP_RETURN, 1);
        candy_proto_t *proto = fs.proto;
        _funcstate_close(&fs, self);
        candy_sclosure_t *closure = candy_sclosure_create(self->ls.gc, self->ls.ctx, proto);
        candy_wrap_t wrap;
        candy_wrap_set_object(&wrap, (candy_object_t *)closure);
        size_t pos = _add_const(self, &wrap);
        _add_iax(self, OP_LOADC, (uint32_t)pos);
        _add_iax(self, OP_SETG, (uint32_t)_add_name_const(self, name));
      } break;
      case TK_if: {
        candy_lexer_next(&self->ls);
        _next(self, '(');
        _expr(self, 0);
        _next(self, ')');
        size_t jump = _add_iax(self, OP_JMPF, 0);
        _block(self, true);
        _next(self, TK_end);
        candy_proto_set_inst(self->fs->proto, jump, (candy_inst_t) {
          .iax = {.op = OP_JMPF, .a = (uint32_t)candy_vector_size(candy_proto_get_inst(self->fs->proto))},
        });
      } break;
      case TK_return:
        candy_lexer_next(&self->ls);
        _expr(self, 0);
        _add_iax(self, OP_RETURN, 1);
        break;
      case TK_IDENT: {
        const candy_array_t *name = _next(self, TK_IDENT)->s;
        if (_test(self, '=')) {
          _expr(self, 0);
          _add_iax(self, OP_SETG, (uint32_t)_add_name_const(self, name));
        }
        else {
          _name_expr(self, name);
          _add_iax(self, OP_POP, 0);
        }
      } break;
      default:
        par_assert(false, "unexpected token %s in statement", candy_token_str(token));
    }
  }
  return CANDY_OK;
}

static void _parser_init(parser_t *self, candy_gc_t *gc, candy_excep_t *ctx, candy_reader_t reader, void *arg) {
  self->fs = NULL;
  candy_lexer_init(&self->ls, gc, ctx, reader, arg);
}

static void _parser_deinit(parser_t *self) {
  candy_lexer_deinit(&self->ls);
  self->fs = NULL;
}

static void _entry(void *arg) {
  parser_t *self = (parser_t *)arg;
  _block(self, false);
  _check(self, TK_EOS);
  size_t none = _add_none(self);
  _add_iax(self, OP_LOADC, (uint32_t)none);
  _add_iax(self, OP_RETURN, 1);
}

candy_err_t candy_parse(candy_gc_t *gc, candy_excep_t *ctx, candy_reader_t reader, void *arg, candy_object_t **out) {
  parser_t prsr;
  funcstate_t fs;
  _parser_init(&prsr, gc, ctx, reader, arg);
  _funcstate_open(&fs, &prsr);
  candy_err_t err = candy_excep_try(ctx, _entry, &prsr, out);
  _funcstate_close(&fs, &prsr);
  _parser_deinit(&prsr);
  if (err == CANDY_OK)
    *out = (candy_object_t *)candy_sclosure_create(gc, ctx, fs.proto);
  return err;
}
