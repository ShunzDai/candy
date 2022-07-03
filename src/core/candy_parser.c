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
  int8_t token;
  int8_t meta[sizeof(candy_meta_t)];
};

typedef struct priv {
  int8_t token;
  int8_t meta[sizeof(candy_meta_t)];
} * priv_t;

struct candy_parser {
  candy_lexer_t lex;
  ast_node_t root;
};

static inline priv_t _private(ast_node_t node) {
  return (priv_t)((struct {ast_node_t l; ast_node_t r; uint8_t args[];} *)node)->args;
}

static ast_node_t _ast_node_create(int8_t token, candy_meta_t *meta, ast_node_t l, ast_node_t r) {
  ast_node_t ast = candy_malloc(sizeof(struct ast_node) + sizeof(int8_t) + sizeof(candy_meta_t));
  ast->l = l;
  ast->r = r;
  _private(ast)->token = token;
  if (meta != NULL)
    (*(candy_meta_t *)_private(ast)->meta) = *meta;
  return ast;
}

static int _ast_node_delete(ast_node_t *ast) {
  candy_assert(ast != NULL);
  if ((*ast)->l != NULL)
    _ast_node_delete(&(*ast)->l);
  if ((*ast)->r != NULL)
    _ast_node_delete(&(*ast)->r);
  candy_free(*ast);
  *ast = NULL;
  return 0;
}

static ast_node_t _term(candy_parser_t parser);
static ast_node_t _factor(candy_parser_t parser);
static ast_node_t _expression(candy_parser_t parser);

static ast_node_t _term(candy_parser_t parser) {
  ast_node_t l = _factor(parser);
  struct priv t = {0};
  t.token = candy_lexer_lookahead(parser->lex);
  while (t.token == '*' || t.token == '/') {
    t.token = candy_lexer_curr(parser->lex, (candy_meta_t *)t.meta);
    l = _ast_node_create(t.token, NULL, l, _factor(parser));
    t.token = candy_lexer_lookahead(parser->lex);
  }
  return l;
}

static ast_node_t _factor(candy_parser_t parser) {
  ast_node_t l = NULL;
  struct priv t = {0};
  t.token = candy_lexer_curr(parser->lex, (candy_meta_t *)t.meta);
  // if (t.token == CANDY_TK_NONE)
  //   return l;
  switch (t.token) {
    case CANDY_TK_CST_INTEGER:
    case CANDY_TK_CST_FLOAT:
      l = _ast_node_create(t.token, (candy_meta_t *)t.meta, NULL, NULL);
      break;
    case '(':
      l = _expression(parser);
      t.token = candy_lexer_curr(parser->lex, (candy_meta_t *)t.meta);
      candy_assert(t.token == ')', "unexpected token: %d", t.token);
      break;
    default:
      candy_assert(0, "unexpected token: %d", t.token);
  }
  return l;
}

static ast_node_t _expression(candy_parser_t parser) {
  ast_node_t l = _term(parser);
  struct priv t = {0};
  t.token = candy_lexer_lookahead(parser->lex);
  while (t.token == '+' || t.token == '-') {
    t.token = candy_lexer_curr(parser->lex, (candy_meta_t *)t.meta);
    l = _ast_node_create(t.token, NULL, l, _term(parser));
    t.token = candy_lexer_lookahead(parser->lex);
  }
  return l;
}

void candy_parser_print(candy_parser_t parser) {
  candy_assert(parser != NULL);

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
