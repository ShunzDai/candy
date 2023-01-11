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
#include "src/candy_lexer.h"
#include <stdlib.h>

#define par_assert(_condition, _format, ...) ((_condition) ? ((void)0U) : candy_throw((self)->lex.buffer, "syntax error: " _format, ##__VA_ARGS__))

struct priv {
  candy_tokens_t token;
  uint8_t wrap[sizeof(candy_wrap_t)];
};

typedef struct candy_parser {
  candy_proto_t *proto;
  /* lexical state */
  candy_lexer_t lex;
} candy_parser_t;

static void _expr(candy_parser_t *self) {
  
}

static void _expr_stat(candy_parser_t *self) {
  candy_wrap_t wrap;
  /* get ident */
  candy_lexer_next(&self->lex, &wrap);
  switch (candy_lexer_lookahead(&self->lex)) {
    case '=':
      while (candy_lexer_next(&self->lex, &wrap));
      break;
    case '(':
      candy_lexer_next(&self->lex, &wrap);
      _expr(self);
      break;
    default:
      par_assert(false, "unknown token %d", candy_lexer_lookahead(&self->lex));
      break;
  }
}

/**
  * @brief  if cond : block { elif cond : block } [ else : block ] end
  * @param  self  parser handle.
  */
static void _cond_block(candy_parser_t *self) {
  /* skip if or elif */
  candy_lexer_next(&self->lex, NULL);
  /** @todo read condition */
  if (candy_lexer_lookahead(&self->lex) == CANDY_TK_break) {

  }
}

static void _if_stat(candy_parser_t *self) {
  /* if cond : block */
  _cond_block(self);
  /* elif cond : block */
  while (candy_lexer_lookahead(&self->lex) == CANDY_TK_elif)
    _cond_block(self);
  /* else : block */
}

static void _statement(candy_parser_t *self) {
  switch (candy_lexer_lookahead(&self->lex)) {
    case CANDY_TK_IDENT:
      _expr_stat(self);
      break;
    case CANDY_TK_if:
      _if_stat(self);
      break;
    case CANDY_TK_while:
      break;
    case CANDY_TK_for:
      break;
    case CANDY_TK_break:
      break;
    default:
      par_assert(false, "unknown token %d", candy_lexer_lookahead(&self->lex));
      break;
  }
}

static bool _block_follow(candy_parser_t *self) {
  switch (candy_lexer_lookahead(&self->lex)) {
    case CANDY_TK_NONE:
      return true;
    default:
      return false;
  }
}

/** @ref https://blog.csdn.net/initphp/article/details/105247775 */
static void _statlist(candy_parser_t *self) {
  while (!_block_follow(self))
    _statement(self);
}

candy_proto_t *candy_parse(candy_buffer_t *buffer, candy_reader_t reader, void *ud) {
  candy_parser_t parser;
  parser.proto = (candy_proto_t *)1;
  candy_lexer_init(&parser.lex, buffer, reader, ud);
  if (candy_try_catch(buffer, (void (*)(void *))_statlist, &parser) != 0)
    goto exit;
  candy_lexer_deinit(&parser.lex);
  return parser.proto;
  exit:
  candy_lexer_deinit(&parser.lex);
  return NULL;
}
