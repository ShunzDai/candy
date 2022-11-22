/**
  * Copyright 2022 ShunzDai
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
#include "src/candy_wrap.h"
#include "src/candy_queue.h"
#include <stdlib.h>

#define lex_assert(_condition, ...) ((_condition) ? ((void)0U) : candy_lexer_assert(self->lex, __VA_ARGS__))

struct ast_node {
  struct ast_node *l;
  struct ast_node *r;
};

struct priv {
  uint8_t token;
  uint8_t wrap[sizeof(candy_wrap_t)];
};

struct candy_parser {
  candy_lexer_t *lex;
  struct ast_node *root;
};

static struct ast_node *_expression(candy_parser_t *self);

static inline struct priv *_private(struct ast_node *node) {
  return (struct priv *)((struct {struct ast_node *l; struct ast_node *r; uint8_t args[];} *)node)->args;
}

static void _ast_node_print(struct ast_node *node) {
  if (node == NULL)
    return;
  _ast_node_print(node->l);
  _ast_node_print(node->r);
  // if (_private(node)->token == CANDY_TK_CST_INTEGER)
  //   printf("CONST\t%ld\ttoken\t%d\n", *(candy_integer_t *)_private(node)->wrap, _private(node)->token);
  // else if (_private(node)->token == CANDY_TK_CST_FLOAT)
  //   printf("CONST\t%.3f\ttoken\t%d\n", *(candy_float_t *)_private(node)->wrap, _private(node)->token);
  // else
  //   printf("%c\n", _private(node)->token);
}

static struct ast_node *_ast_node_create(int8_t token, candy_wrap_t *wrap, struct ast_node *l, struct ast_node *r) {
  struct ast_node *node = (struct ast_node *)malloc(sizeof(struct ast_node) + ((wrap == NULL) ? sizeof(int8_t) : sizeof(struct priv)));
  node->l = l;
  node->r = r;
  _private(node)->token = token;
  if (wrap != NULL)
    *(candy_wrap_t *)_private(node)->wrap = *wrap;
  return node;
}

static int _ast_node_delete(struct ast_node **node) {
  if ((*node)->l != NULL)
    _ast_node_delete(&(*node)->l);
  if ((*node)->r != NULL)
    _ast_node_delete(&(*node)->r);
  free(*node);
  *node = NULL;
  return 0;
}

static struct ast_node *_factor(candy_parser_t *self) {
  struct ast_node *l = NULL;
  union candy_wrap wrap;
  int sign = 1;
  uint8_t token = CANDY_TK_NONE;
  begin:
  token = candy_lexer_next(self->lex, &wrap);
  switch (token) {
    case '-':
      sign = -1;
    case '+':
      token = candy_lexer_lookahead(self->lex);
      lex_assert(token == CANDY_TK_CST_INTEGER || token == CANDY_TK_CST_FLOAT, "unexpected token '%c'(0x%02X)", token, token);
      goto begin;
    case CANDY_TK_CST_INTEGER:
      *candy_wrap_get_integer(&wrap, NULL) *= sign;
      break;
    case CANDY_TK_CST_FLOAT:
      *candy_wrap_get_float(&wrap, NULL) *= sign;
      break;
    case '(':
      l = _expression(self);
      token = candy_lexer_next(self->lex, &wrap);
      lex_assert(token == ')', "unexpected token '%c'(0x%02X)", token, token);
      return l;
    default:
      lex_assert(0, "unexpected token '%c'(0x%02X)", token, token);
  }
  return _ast_node_create(token, &wrap, NULL, NULL);
}

static struct ast_node *_term(candy_parser_t *self) {
  struct ast_node *l = _factor(self);
  for (uint8_t token = candy_lexer_lookahead(self->lex); token == '*' || token == '/'; token = candy_lexer_lookahead(self->lex)) {
    token = candy_lexer_next(self->lex, NULL);
    l = _ast_node_create(token, NULL, l, _factor(self));
  }
  return l;
}

static struct ast_node *_expression(candy_parser_t *self) {
  struct ast_node *l = _term(self);
  for (uint8_t token = candy_lexer_lookahead(self->lex); token == '+' || token == '-'; token = candy_lexer_lookahead(self->lex)) {
    token = candy_lexer_next(self->lex, NULL);
    l = _ast_node_create(token, NULL, l, _term(self));
  }
  return l;
}

void candy_parser_print(candy_parser_t *self) {
  _ast_node_print(self->root);
}

candy_parser_t *candy_parse(candy_reader_t reader, void *ud) {
  candy_parser_t *self = (candy_parser_t *)malloc(sizeof(struct candy_parser));
  jmp_buf buf;
  self->lex = candy_lexer_create(&buf, reader, ud);
  if(setjmp(buf))
    goto exit;
  self->root = _expression(self);
  return self;
  exit:
  candy_parser_delete(&self);
  return self;
}

int candy_parser_delete(candy_parser_t **self) {
  candy_lexer_delete(&(*self)->lex);
  _ast_node_delete(&(*self)->root);
  free(*self);
  *self = NULL;
  return 0;
}
