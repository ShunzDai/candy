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
#include "core/candy_parser.h"
#include "core/candy_exception.h"
#include "core/candy_gc.h"
#include "core/candy_proto.h"
#include "core/candy_closure.h"
#include "core/candy_array.h"
#include "core/candy_lexer.h"

/**
 * @brief  parser for candy language.
 */

#define par_assert(_condition, _format, ...) \
candy_assert(self->ls.ctx, self->ls.gc, _condition, CANDY_ERR_SYNTAX, \
"line %zu col %zu: " _format, self->ls.dbg.line, self->ls.dbg.column, ##__VA_ARGS__)

typedef struct expdesc expdesc_t;
typedef struct lh_assign lh_assign_t;
typedef struct funcstate funcstate_t;
typedef struct parser parser_t;

typedef enum exptype {
  EXP_TYPE_NONE,
  EXP_TYPE_CALL,
  EXP_TYPE_VOID,
} exptype_t;

struct expdesc {
  exptype_t type;
};

struct lh_assign {
  lh_assign_t *prev;
  expdesc_t v;
};

struct funcstate {
  funcstate_t *prev;
  candy_proto_t *proto;
};

struct parser {
  /* lexical state */
  candy_lexer_t ls;
  funcstate_t *fs;
};

static const char TAG[] = "parser";

static void _statement(parser_t *self);

static void _funcstate_open(funcstate_t *self, parser_t *prsr) {
  candy_logd(TAG, "open funcstate, prev %p, next %p", prsr->fs, self);
  self->prev = prsr->fs;
  self->proto = candy_proto_create(prsr->ls.gc, prsr->ls.ctx);
  prsr->fs = self;
}

static void _funcstate_close(funcstate_t *self, parser_t *prsr) {
  candy_logd(TAG, "close funcstate, prev %p, next %p", prsr->fs, self->prev);
  prsr->fs = self->prev;
}

static int _search_local(funcstate_t *self, const candy_array_t *id, expdesc_t *e) {
  return -1;
}

static int _search_upvalue(funcstate_t *self, const candy_array_t *id, expdesc_t *e) {
  return -1;
}

static void _var_single_aux(funcstate_t *self, const candy_array_t *id, expdesc_t *e) {
  if (self) {
    int idx = _search_local(self, id, e);
    if (idx < 0) {
      idx = _search_upvalue(self, id, e);
      if (idx < 0) {
        _var_single_aux(self->prev, id, e);
      }
    }
    else {

    }
  }
  else {
    e->type = EXP_TYPE_NONE;
  }
}

static void _parser_init(parser_t *self, candy_gc_t *gc, candy_excep_t *ctx, candy_reader_t reader, void *arg) {
  self->fs = NULL;
  candy_lexer_init(&self->ls, gc, ctx, reader, arg);
}

static void _parser_deinit(parser_t *self) {
  candy_lexer_deinit(&self->ls);
  self->fs = NULL;
}

static void _check(parser_t *self, candy_tokens_t token) {
  par_assert(candy_lexer_lookahead(&self->ls) == token, "unexpected token");
}

static const candy_meta_t *_check_next(parser_t *self, candy_tokens_t token) {
  _check(self, token);
  return candy_lexer_next(&self->ls);
}

static bool _test_next(parser_t *self, candy_tokens_t token) {
  if (candy_lexer_lookahead(&self->ls) == token) {
    candy_lexer_next(&self->ls);
    return true;
  }
  return false;
}

static void _check_match(parser_t *self, candy_tokens_t begin, candy_tokens_t end, size_t at) {
  bool res = _test_next(self, end);
  par_assert(res, "%s was never closed with %s", candy_token_str(begin), candy_token_str(end));
}

static const candy_array_t *_check_ident(parser_t *self) {
  _check(self, TK_IDENT);
  const candy_array_t *id = candy_lexer_next(&self->ls)->s;
  candy_lexer_lookahead(&self->ls);
  return id;
}

/* param list -> [param {',' param}] */
static void _param_list(parser_t *self) {
  while (1) {
    switch (candy_lexer_lookahead(&self->ls)) {
      case TK_IDENT:
        break;
      default:
        break;
    }
  }
}

static void _block(parser_t *self) {
  _statement(self);
}

/* '(' param list ')' */
static void _body(parser_t *self, expdesc_t *args) {
  funcstate_t fs;
  _funcstate_open(&fs, self);
  _check_next(self, '(');
  _param_list(self);
  _check_next(self, ')');
  _block(self);
  _funcstate_close(&fs, self);
}

static void _var_single(parser_t *self, expdesc_t *e) {
  const candy_array_t *id = _check_ident(self);
  _var_single_aux(self->fs, id, e);
  if (e->type == EXP_TYPE_NONE) {

  }
}

static void _subexpr(parser_t *self, expdesc_t *e, int limit) {
}

static void _expr(parser_t *self, expdesc_t *e) {
  _subexpr(self, e, 0);
}

static void _expr_primary(parser_t *self, expdesc_t *e) {
  switch(candy_lexer_lookahead(&self->ls)) {
    case '(': {
      size_t line = self->ls.dbg.line;
      candy_lexer_next(&self->ls);
      _expr(self, e);
      _check_match(self, '(', ')', line);
    } break;
    case TK_IDENT:
      _var_single(self, e);
      break;
    default:
      par_assert(0, "unexpected symbol");
      break;
  }
}

static void _expr_suffixed(parser_t *self, expdesc_t *e) {
  expdesc_t exp;
  _expr_primary(self, e);
    switch (candy_lexer_lookahead(&self->ls)) {
      case '(':
        candy_lexer_next(&self->ls);
        if (candy_lexer_lookahead(&self->ls) == ')') {
          exp.type = EXP_TYPE_VOID;
        }
        candy_lexer_next(&self->ls);
        break;
      default:
        break;
    }
}

static void _stat_func(parser_t *self) {
  expdesc_t args;
  /* skip 'def' */
  candy_lexer_next(&self->ls);
  _body(self, &args);
}

/**
  * @brief  if '(' expr ')' block { elif '(' expr ')' block } [ else block ] end
  * @param  self  parser handle.
  */
static void _stat_if(parser_t *self) {
  /* if '(' expr ')' block */
  /* { elif '(' expr ')' block } */
  /* [ else block ] end */
}

static void _stat_expr(parser_t *self) {
  lh_assign_t v;
  _expr_suffixed(self, &v.v);
  if (candy_lexer_lookahead(&self->ls) == '=' || candy_lexer_lookahead(&self->ls) == ',') {
    v.prev = NULL;

  }
  else {
    par_assert(v.v.type == EXP_TYPE_CALL, "syntax error");
    // candy_proto_add_iabc(self->fs->proto, OP_CALL);
  }
}

static void _statement(parser_t *self) {
  switch (candy_lexer_lookahead(&self->ls)) {
    case TK_EOS:
      break;
    case TK_def:
      _stat_func(self);
      break;
    case TK_if:
      _stat_if(self);
      break;
    case TK_while:
      break;
    case TK_for:
      break;
    case TK_break:
      break;
    default:
      _stat_expr(self);
      break;
  }
}

static void _entry(parser_t *self) {
  _statement(self);
  _check(self, TK_EOS);
}

candy_err_t candy_parse(candy_gc_t *gc, candy_excep_t *ctx, candy_reader_t reader, void *arg, candy_object_t **out) {
  parser_t prsr;
  funcstate_t fs;
  _parser_init(&prsr, gc, ctx, reader, arg);
  _funcstate_open(&fs, &prsr);
  candy_err_t err = candy_excep_try(ctx, (candy_excep_cb_t)_entry, &prsr, out);
  _funcstate_close(&fs, &prsr);
  _parser_deinit(&prsr);
  if (err == CANDY_OK)
    *out = (candy_object_t *)candy_sclosure_create(gc, ctx, fs.proto);
  return err;
}
