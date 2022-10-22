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
#include "candy_parser.h"
#include "candy_lexer.h"
#include "candy_wrap.h"
#include "candy_queue.h"
#include "candy_platform.h"
#include <stdlib.h>

struct ast_node {
  struct ast_node *l;
  struct ast_node *r;
};

struct priv {
  uint8_t token;
  uint8_t meta[sizeof(candy_meta_t)];
};

struct candy_parser {
  struct candy_lexer *lex;
  struct ast_node *root;
};

static struct ast_node *_expression(struct candy_parser *parser);

static inline struct priv *_private(struct ast_node *node) {
  return (struct priv *)((struct {struct ast_node *l; struct ast_node *r; uint8_t args[];} *)node)->args;
}

static void _ast_node_print(struct ast_node *node) {
  if (node == NULL)
    return;
  _ast_node_print(node->l);
  _ast_node_print(node->r);
  // if (_private(node)->token == CANDY_TK_CST_INTEGER)
  //   printf("CONST\t%ld\ttoken\t%d\n", *(candy_integer_t *)_private(node)->meta, _private(node)->token);
  // else if (_private(node)->token == CANDY_TK_CST_FLOAT)
  //   printf("CONST\t%.3f\ttoken\t%d\n", *(candy_float_t *)_private(node)->meta, _private(node)->token);
  // else
  //   printf("%c\n", _private(node)->token);
}

static struct ast_node *_ast_node_create(int8_t token, candy_meta_t *meta, struct ast_node *l, struct ast_node *r) {
  struct ast_node *node = (ast_node_t)malloc(sizeof(struct ast_node) + ((meta == NULL) ? sizeof(int8_t) : sizeof(struct priv)));
  node->l = l;
  node->r = r;
  _private(node)->token = token;
  if (meta != NULL)
    *(candy_meta_t *)_private(node)->meta = *meta;
  return node;
}

static int _ast_node_delete(struct ast_node **node) {
  candy_assert(node != NULL);
  if ((*node)->l != NULL)
    _ast_node_delete(&(*node)->l);
  if ((*node)->r != NULL)
    _ast_node_delete(&(*node)->r);
  free(*node);
  *node = NULL;
  return 0;
}

static struct ast_node *_factor(struct candy_parser *parser) {
  struct ast_node *l = NULL;
  union candy_meta meta;
  int sign = 1;
  uint8_t token = CANDY_TK_NONE;
  begin:
  token = candy_lexer_curr(parser->lex, &meta);
  switch (token) {
    case '-':
      sign = -1;
    case '+':
      token = candy_lexer_lookahead(parser->lex);
      candy_assert(token == CANDY_TK_CST_INTEGER || token == CANDY_TK_CST_FLOAT, "unexpected token: %c", token);
      goto begin;
    case CANDY_TK_CST_INTEGER:
      meta.i *= sign;
      break;
    case CANDY_TK_CST_FLOAT:
      meta.f *= sign;
      break;
    case '(':
      l = _expression(parser);
      token = candy_lexer_curr(parser->lex, &meta);
      candy_assert(token == ')', "unexpected token: %d", token);
      return l;
    default:
      candy_assert(0, "unexpected token: %c", token);
  }
  return _ast_node_create(token, &meta, NULL, NULL);
}

static struct ast_node *_term(struct candy_parser *parser) {
  struct ast_node *l = _factor(parser);
  for (uint8_t token = candy_lexer_lookahead(parser->lex); token == '*' || token == '/'; token = candy_lexer_lookahead(parser->lex)) {
    token = candy_lexer_curr(parser->lex, NULL);
    l = _ast_node_create(token, NULL, l, _factor(parser));
  }
  return l;
}

static struct ast_node *_expression(struct candy_parser *parser) {
  struct ast_node *l = _term(parser);
  for (uint8_t token = candy_lexer_lookahead(parser->lex); token == '+' || token == '-'; token = candy_lexer_lookahead(parser->lex)) {
    token = candy_lexer_curr(parser->lex, NULL);
    l = _ast_node_create(token, NULL, l, _term(parser));
  }
  return l;
}

void candy_parser_print(struct candy_parser *parser) {
  candy_assert(parser != NULL);
  _ast_node_print(parser->root);
}

struct candy_parser *candy_parser_create(const char code[]) {
  struct candy_parser *parser = (candy_parser_t)malloc(sizeof(struct candy_parser));
  uint8_t buffer[1026] = {0x00, 0x04};
  parser->lex = candy_lexer_create(code, (struct candy_view *)buffer);
  parser->root = _expression(parser);
  return parser;
}

int candy_parser_delete(struct candy_parser **parser) {
  candy_assert(parser != NULL);
  candy_assert(*parser != NULL);
  candy_lexer_delete(&(*parser)->lex);
  _ast_node_delete(&(*parser)->root);
  free(*parser);
  *parser = NULL;
  return 0;
}
