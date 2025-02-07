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
#include "core/candy_print.h"
#include "core/candy_lexer.h"

#define par_assert(_condition, _format, ...) \
candy_assert(self->ls.ctx, self->ls.gc, _condition, CANDY_ERR_SYNTAX, _format, ##__VA_ARGS__)

typedef struct candy_funcstate funcstate_t;
typedef struct candy_parser parser_t;
typedef struct candy_expdesc expdesc_t;

struct candy_expdesc {

};

struct candy_funcstate {
  funcstate_t *prev;
  candy_proto_t *proto;
};

struct candy_parser {
  /* lexical state */
  candy_lexer_t ls;
  funcstate_t *fs;
};

static const char TAG[] = "candy::parser";

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

static void _parser_init(parser_t *self, candy_gc_t *gc, candy_excep_t *ctx, candy_reader_t reader, void *arg) {
  self->fs = NULL;
  candy_lexer_init(&self->ls, gc, ctx, reader, arg);
}

static void _parser_deinit(parser_t *self) {
  candy_lexer_deinit(&self->ls);
  self->fs = NULL;
}

static void _fs_init(funcstate_t *self, parser_t *par) {
  self->prev = par->fs;
  self->proto = candy_proto_create(par->ls.gc, par->ls.ctx);
  par->fs = self;
}

static void _expr(parser_t *self, expdesc_t *e) {
  while (1) {
    switch (candy_lexer_lookahead(&self->ls)) {
      case ')':
        return;
      case TK_STRING:
      case TK_INTEGER:
      case TK_FLOAT:
        break;
      default:
        par_assert(false, "unknown token %s", candy_token_str(candy_lexer_lookahead(&self->ls)));
        break;
    }
  }
}

/* lambda '(' expr ')' */
static void _expr_lambda(parser_t *self) {
  expdesc_t e;
  /* skip '(' */
  candy_lexer_next(&self->ls);
  _expr(self, &e);
  /* skip ')' */
  candy_lexer_next(&self->ls);
}

static void stat_def(parser_t *self) {
  funcstate_t fs;
  _fs_init(&fs, self);
  /* skip def */
  candy_lexer_next(&self->ls);
  switch (candy_lexer_lookahead(&self->ls)) {
    case TK_IDENT:
      candy_lexer_next(&self->ls);
      par_assert(candy_lexer_lookahead(&self->ls) == '(', "unknown token %s", candy_token_str(candy_lexer_lookahead(&self->ls)));
      _expr_lambda(self);
      break;
    /* lambda expression */
    case '(':
      _expr_lambda(self);
      break;
    default:
      par_assert(false, "unknown token %s", candy_token_str(candy_lexer_lookahead(&self->ls)));
  }
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

/** @ref https://blog.csdn.net/initphp/article/details/105247775 */
static void _statement(parser_t *self) {
  while (candy_lexer_lookahead(&self->ls) != TK_EOS) {
    switch (candy_lexer_lookahead(&self->ls)) {
      case TK_def:
          stat_def(self);
          break;
      // case TK_if:
      //   // _stat_if(self);
      //   break;
      // case TK_while:
      //   break;
      // case TK_for:
      //   break;
      // case TK_break:
      //   break;
      default:
        // par_assert(false, "unknown token %d", candy_lexer_lookahead(&self->lex));
        // printf("%s line %zu col %zu\n", candy_token_str(candy_lexer_lookahead(&self->ls)), self->ls.dbg.line, self->ls.dbg.column);
        candy_lexer_next(&self->ls);
        break;
    }
  }
}

candy_err_t candy_parse(candy_gc_t *gc, candy_reader_t reader, void *arg, candy_object_t **out) {
  candy_excep_t ctx;
  parser_t prsr;
  funcstate_t fs;
  candy_excep_init(&ctx);
  _parser_init(&prsr, gc, &ctx, reader, arg);
  _funcstate_open(&fs, &prsr);
  candy_err_t err = candy_excep_try(&ctx, (candy_excep_cb_t)_statement, &prsr, out);
  _funcstate_close(&fs, &prsr);
  _parser_deinit(&prsr);
  candy_excep_deinit(&ctx);
  if (err == CANDY_OK)
    *out = (candy_object_t *)candy_sclosure_create(gc, &ctx, fs.proto);
  return err;
}
