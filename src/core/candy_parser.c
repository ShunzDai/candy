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
#include "src/core/candy_lexer.h"
#include "src/platform/candy_memory.h"
#include "src/struct/candy_wrap.h"
#include "src/struct/candy_queue.h"

struct ast_node {
  ast_node_t l;
  ast_node_t r;
};

typedef struct priv {
  int8_t token;
  int8_t meta[sizeof(candy_meta_t)];
} * priv_t;

struct candy_parser {
  candy_lexer_t lex;
  ast_node_t root;
};

static ast_node_t _expression(candy_parser_t parser);

static inline priv_t _private(ast_node_t node) {
  return (priv_t)((struct {ast_node_t l; ast_node_t r; uint8_t args[];} *)node)->args;
}

static void _ast_node_print(ast_node_t node) {
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

static ast_node_t _ast_node_create(int8_t token, candy_meta_t *meta, ast_node_t l, ast_node_t r) {
  ast_node_t node = (ast_node_t)candy_malloc(sizeof(struct ast_node) + ((meta == NULL) ? sizeof(int8_t) : sizeof(struct priv)));
  node->l = l;
  node->r = r;
  _private(node)->token = token;
  if (meta != NULL)
    *(candy_meta_t *)_private(node)->meta = *meta;
  return node;
}

static int _ast_node_delete(ast_node_t *node) {
  candy_assert(node != NULL);
  if ((*node)->l != NULL)
    _ast_node_delete(&(*node)->l);
  if ((*node)->r != NULL)
    _ast_node_delete(&(*node)->r);
  candy_free(*node);
  *node = NULL;
  return 0;
}

static ast_node_t _factor(candy_parser_t parser) {
  ast_node_t l = NULL;
  candy_meta_t meta;
  int8_t token = candy_lexer_curr(parser->lex, &meta);
  switch (token) {
    case CANDY_TK_CST_INTEGER:
    case CANDY_TK_CST_FLOAT:
      l = _ast_node_create(token, &meta, NULL, NULL);
      break;
    case '(':
      l = _expression(parser);
      token = candy_lexer_curr(parser->lex, &meta);
      candy_assert(token == ')', "unexpected token: %d", token);
      break;
    default:
      candy_assert(0, "unexpected token: %c", token);
  }
  return l;
}

static ast_node_t _term(candy_parser_t parser) {
  ast_node_t l = _factor(parser);
  for (int8_t token = candy_lexer_lookahead(parser->lex); token == '*' || token == '/'; token = candy_lexer_lookahead(parser->lex)) {
    token = candy_lexer_curr(parser->lex, NULL);
    l = _ast_node_create(token, NULL, l, _factor(parser));
  }
  return l;
}

static ast_node_t _expression(candy_parser_t parser) {
  ast_node_t l = _term(parser);
  for (int8_t token = candy_lexer_lookahead(parser->lex); token == '+' || token == '-'; token = candy_lexer_lookahead(parser->lex)) {
    token = candy_lexer_curr(parser->lex, NULL);
    l = _ast_node_create(token, NULL, l, _term(parser));
  }
  return l;
}

void candy_parser_print(candy_parser_t parser) {
  candy_assert(parser != NULL);
  _ast_node_print(parser->root);
}

candy_parser_t candy_parser_create(const char code[]) {
  candy_parser_t parser = (candy_parser_t)candy_malloc(sizeof(struct candy_parser));
  parser->lex = candy_lexer_create(code);
  parser->root = _expression(parser);
  return parser;
}

int candy_parser_delete(candy_parser_t *parser) {
  candy_assert(parser != NULL);
  candy_assert(*parser != NULL);
  candy_lexer_delete(&(*parser)->lex);
  _ast_node_delete(&(*parser)->root);
  candy_free(*parser);
  *parser = NULL;
  return 0;
}
