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

static void _block(parser_t *self);

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

static void _add_localvar(funcstate_t *self, candy_array_t *var, int idx) {

}

static void _parser_init(parser_t *self, candy_gc_t *gc, candy_excep_t *ctx, candy_reader_t reader, void *arg) {
  self->fs = NULL;
  candy_lexer_init(&self->ls, gc, ctx, reader, arg);
}

static void _parser_deinit(parser_t *self) {
  candy_lexer_deinit(&self->ls);
  self->fs = NULL;
}

// static void _parser_add_constv(parser_t *self, candy_wrap_t *constv) {
//   candy_vector_append(candy_proto_get_const(self->fs->proto), candy_gc_memory(self->ls.gc), self->ls.ctx, constv, sizeof(candy_wrap_t));
// }

static void _check(parser_t *self, candy_tokens_t token) {
  par_assert(candy_lexer_lookahead(&self->ls) == token, "unexpected token");
}

static const candy_meta_t *_check_next(parser_t *self, candy_tokens_t token) {
  _check(self, token);
  return candy_lexer_next(&self->ls);
}

/* param list -> [param {',' param}] */
static void _param_list(parser_t *self) {
  int nparams = 0;
  while (1) {
    switch (candy_lexer_lookahead(&self->ls)) {
      case TK_IDENT:
        _add_localvar(self->fs, _check_next(self, TK_IDENT)->s, nparams++);
        break;
      default:
        break;
    }
  }
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

static void _prefix_expr(parser_t *self, expdesc_t *e) {
  switch(candy_lexer_lookahead(&self->ls)) {
    case TK_IDENT:
      break;
    default:
      break;
  }
}

static void _primary_expr(parser_t *self, expdesc_t *e) {
  _prefix_expr(self, e);
}

static void _stat_expr(parser_t *self) {
  lh_assign_t v;
  _primary_expr(self, &v.v);
}

static void _stat_func(parser_t *self) {
  expdesc_t args;
  /* skip 'def' */
  candy_lexer_next(&self->ls);
  _body(self, &args);
}

// /**
//   * @brief  if '(' expr ')' block { elif '(' expr ')' block } [ else block ] end
//   * @param  self  parser handle.
//   */
// static void stat_if(parser_t *self) {
//   /* if '(' expr ')' block */
//   /* { elif '(' expr ')' block } */
//   /* [ else block ] end */
// }

static void _statement(parser_t *self) {
  while (1) {
    switch (candy_lexer_lookahead(&self->ls)) {
      case TK_EOS:
        return;
      case TK_def:
        _stat_func(self);
        break;
      // case TK_if:
      //   _stat_if(self);
      //   break;
      // case TK_while:
      //   break;
      // case TK_for:
      //   break;
      // case TK_break:
      //   break;
      default:
        _stat_expr(self);
        break;
    }
  }
}

static void _block(parser_t *self) {
  _statement(self);
}

candy_sclosure_t *candy_parse(candy_gc_t *gc, candy_excep_t *ctx, candy_reader_t reader, void *arg) {
  parser_t prsr;
  funcstate_t fs;
  _parser_init(&prsr, gc, ctx, reader, arg);
  _funcstate_open(&fs, &prsr);
  _block(&prsr);
  _funcstate_close(&fs, &prsr);
  _parser_deinit(&prsr);
  return candy_sclosure_create(gc, ctx, fs.proto);
}
