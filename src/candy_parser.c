/**
  * Copyright 2022-2024 ShunzDai
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
#include "src/candy_parser.h"
#include "src/candy_lexer.h"
#include "src/candy_gc.h"
#include "src/candy_array.h"
#include "src/candy_proto.h"
#include "src/candy_closure.h"

#define par_assert(_condition, _format, ...) candy_assert(&self->ls.ctx, self->ls.gc, _condition, syntax, _format, ##__VA_ARGS__)

typedef struct candy_funcstate candy_funcstate_t;
typedef struct candy_parser candy_parser_t;
typedef struct candy_expdesc candy_expdesc_t;

struct candy_expdesc {

};

struct candy_funcstate {
  candy_funcstate_t *prev;
  candy_proto_t *proto;
};

struct candy_parser {
  /* lexical state */
  candy_lexer_t ls;
  candy_funcstate_t *fs;
};

static void expr(candy_parser_t *self, candy_expdesc_t *e) {
  while (1) {
    switch (candy_lexer_lookahead(&self->ls)) {
      case ')':
        return;
      case TK_STRING:
      case TK_INTEGER:
      case TK_FLOAT:
        break;
      default:
        par_assert(false, "unknown token %d", candy_lexer_lookahead(&self->ls));
        break;
    }
  }
}

/* lambda '(' expr ')' */
static void expr_lambda(candy_parser_t *self) {
  candy_expdesc_t e;
  // candy_proto_t *b = NULL;
  /* skip '(' */
  candy_lexer_next(&self->ls);
  expr(self, &e);
  /* skip ')' */
  candy_lexer_next(&self->ls);
}

/*  */
static void stat_def(candy_parser_t *self) {
  /* skip def */
  candy_lexer_next(&self->ls);
  switch (candy_lexer_lookahead(&self->ls)) {
    case TK_IDENT:
      candy_lexer_next(&self->ls);
      par_assert(candy_lexer_lookahead(&self->ls) == '(', "unknown token %d", candy_lexer_lookahead(&self->ls));
      expr_lambda(self);
      break;
    /* lambda expression */
    case '(':
      expr_lambda(self);
      break;
    default:
      par_assert(false, "unknown token %d", candy_lexer_lookahead(&self->ls));
  }
}

// /**
//   * @brief  if '(' expr ')' block { elif '(' expr ')' block } [ else block ] end
//   * @param  self  parser handle.
//   */
// static void stat_if(candy_parser_t *self) {
//   /* if '(' expr ')' block */
//   /* { elif '(' expr ')' block } */
//   /* [ else block ] end */
// }

/** @ref https://blog.csdn.net/initphp/article/details/105247775 */
static void _statement(candy_parser_t *self) {
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
        printf("%s line %zu col %zu\n", candy_token_str(candy_lexer_lookahead(&self->ls)), self->ls.dbg.line, self->ls.dbg.column);
        candy_lexer_next(&self->ls);
        break;
    }
  }
}

candy_object_t *candy_parse(candy_gc_t *gc, candy_reader_t reader, void *arg) {
  candy_funcstate_t fs = {
    .prev = NULL,
    .proto = candy_proto_create(gc),
  };
  candy_parser_t parser = {
    .fs = &fs,
  };
  candy_lexer_init(&parser.ls, gc, reader, arg);
  candy_array_t *err = candy_exce_try(&parser.ls.ctx, (candy_exce_cb_t)_statement, &parser);
  candy_lexer_deinit(&parser.ls);
  if (err)
    return (candy_object_t *)err;
  return (candy_object_t *)candy_sclosure_create(gc, fs.proto);
}
