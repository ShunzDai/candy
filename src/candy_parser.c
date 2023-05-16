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
#include "src/candy_parser.h"
#include "src/candy_io.h"
#include "src/candy_lexer.h"
#include "src/candy_block.h"
#include <stdlib.h>

#define par_assert(_condition, _format, ...) candy_assert(_condition, syntax, _format, ##__VA_ARGS__)

typedef struct candy_parser {
  /* lexical state */
  candy_lexer_t lex;
  candy_block_t *head;
} candy_parser_t;

static void _expr(candy_parser_t *self) {
  candy_lexer_next(&self->lex);
  switch (candy_lexer_lookahead(&self->lex)) {
    case ')':
      candy_lexer_next(&self->lex);
      break;
    case TK_INTEGER:
    case TK_FLOAT:
    case TK_STRING:
      candy_block_add_const(self->head, candy_lexer_next(&self->lex));
      break;
    default:
      par_assert(false, "unknown token");
      break;
  }
}

static void _expr_stat(candy_parser_t *self) {
  /* get ident */
  candy_block_add_const(self->head, candy_lexer_next(&self->lex));
  candy_block_add_iabc(self->head, OP_GETTABUP, 0, 0, self->head->pool.size - 1);
  switch (candy_lexer_lookahead(&self->lex)) {
    case '=':
      break;
    case '(':
      _expr(self);
      candy_block_add_iabc(self->head, OP_CALL, 0, 0, 0);
      break;
    default:
      par_assert(false, "identifiers can only be used for assignment or invocation");
      break;
  }
}

/**
  * @brief  if cond : block { elif cond : block } [ else : block ] end
  * @param  self  parser handle.
  */
static void _cond_block(candy_parser_t *self) {
  /* skip if or elif */
  candy_lexer_next(&self->lex);
  /** @todo read condition */
  if (candy_lexer_lookahead(&self->lex) == TK_break) {

  }
}

static void _if_stat(candy_parser_t *self) {
  /* if cond : block */
  _cond_block(self);
  /* elif cond : block */
  while (candy_lexer_lookahead(&self->lex) == TK_elif)
    _cond_block(self);
  /* else : block */
}

/** @ref https://blog.csdn.net/initphp/article/details/105247775 */
static void _statement(candy_parser_t *self, void *ud) {
  while (candy_lexer_lookahead(&self->lex) != TK_EOS) {
    switch (candy_lexer_lookahead(&self->lex)) {
      case TK_IDENT:
        _expr_stat(self);
        break;
      case TK_if:
        _if_stat(self);
        break;
      case TK_while:
        break;
      case TK_for:
        break;
      case TK_break:
        break;
      default:
        par_assert(false, "unknown token %d", candy_lexer_lookahead(&self->lex));
        break;
    }
  }
}

candy_block_t *candy_parse(candy_io_t *io, candy_reader_t reader, void *ud) {
  candy_parser_t parser;
  candy_lexer_init(&parser.lex, io, reader, ud);
  parser.head = candy_block_create();
  if (candy_io_try_catch(io, (candy_try_catch_cb_t)_statement, &parser, NULL) != 0)
    goto exit;
  return parser.head;
  exit:
  candy_block_delete(&parser.head);
  candy_lexer_deinit(&parser.lex);
  return NULL;
}
