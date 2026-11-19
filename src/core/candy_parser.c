/**
  * Copyright 2022-2026 ShunzDai
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
#include "core/candy_wrap.h"
#include "core/candy_gc.h"
#include "core/candy_proto.h"
#include "core/candy_closure.h"
#include "core/candy_array.h"
#include "core/candy_lexer.h"
#include <string.h>

#define par_assert(_condition, _format, ...) \
candy_assert(self->ls.ctx, self->ls.gc, _condition, CANDY_ERR_SYNTAX, \
"line %zu col %zu: " _format, self->ls.dbg.line, self->ls.dbg.column, ##__VA_ARGS__)

typedef struct loopstate loopstate_t;
typedef struct funcstate funcstate_t;
typedef struct parser parser_t;

struct loopstate {
  loopstate_t *prev;
  size_t continue_pc;
  size_t breaks;
  size_t continues;
};

struct funcstate {
  funcstate_t *prev;
  candy_proto_t *proto;
  const candy_array_t *name;
  const candy_array_t *locals[UINT8_MAX];
  uint8_t local_reg[UINT8_MAX];
  uint8_t nlocal;
  uint8_t freereg;
  uint8_t local_hwm;
  uint8_t nreg;
  loopstate_t *loop;
};

struct parser {
  candy_lexer_t ls;
  funcstate_t *fs;
};

static int _expr(parser_t *self, int pref, int min_prec);
static bool _block(parser_t *self, bool stop_at_end);

static void _funcstate_open(funcstate_t *self, parser_t *prsr) {
  self->prev = prsr->fs;
  self->proto = candy_proto_create(prsr->ls.gc, prsr->ls.ctx);
  self->name = NULL;
  self->nlocal = 0;
  self->freereg = 1;
  self->local_hwm = 1;
  self->nreg = 1;
  self->loop = NULL;
  prsr->fs = self;
}

static void _funcstate_close(funcstate_t *self, parser_t *prsr) {
  prsr->fs = self->prev;
}

static int _newreg(parser_t *self) {
  par_assert(self->fs->freereg < UINT8_MAX, "too many registers");
  int reg = self->fs->freereg++;
  if (self->fs->freereg > self->fs->nreg)
    self->fs->nreg = self->fs->freereg;
  return reg;
}

static size_t _add_const(parser_t *self, candy_wrap_t *wrap) {
  return candy_proto_add_const(self->fs->proto, self->ls.gc, self->ls.ctx, wrap);
}

static size_t _add_name_const(parser_t *self, const candy_array_t *name) {
  candy_wrap_t wrap;
  candy_wrap_set_object(&wrap, (const candy_object_t *)name);
  return _add_const(self, &wrap);
}

static size_t _add_integer(parser_t *self, candy_integer_t val) {
  candy_wrap_t wrap;
  candy_wrap_set_integer(&wrap, val);
  return _add_const(self, &wrap);
}

static bool _is_literal(candy_tokens_t token) {
  return token == TK_INTEGER || token == TK_FLOAT || token == TK_STRING ||
    token == TK_true || token == TK_false || token == TK_null;
}

static candy_wrap_t _literal_wrap(parser_t *self, candy_tokens_t token) {
  candy_wrap_t wrap;
  switch (token) {
    case TK_INTEGER: candy_wrap_set_integer(&wrap, candy_lexer_next(&self->ls)->i); break;
    case TK_FLOAT:   candy_wrap_set_float(&wrap, candy_lexer_next(&self->ls)->f); break;
    case TK_STRING:  candy_wrap_set_object(&wrap, (const candy_object_t *)candy_lexer_next(&self->ls)->s); break;
    case TK_true:    candy_lexer_next(&self->ls); candy_wrap_set_boolean(&wrap, true); break;
    case TK_false:   candy_lexer_next(&self->ls); candy_wrap_set_boolean(&wrap, false); break;
    case TK_null:
      candy_lexer_next(&self->ls);
      candy_wrap_set_type(&wrap, CANDY_TYPE_NULL);
      candy_wrap_set_mask(&wrap, MASK_CONST);
      break;
    default:
      par_assert(false, "unknown expression %s", candy_token_str(token));
      break;
  }
  return wrap;
}

static size_t _add_meta(parser_t *self, candy_tokens_t token) {
  candy_wrap_t wrap = _literal_wrap(self, token);
  return _add_const(self, &wrap);
}

static size_t _add_null(parser_t *self) {
  candy_wrap_t wrap;
  candy_wrap_set_type(&wrap, CANDY_TYPE_NULL);
  candy_wrap_set_mask(&wrap, MASK_CONST);
  return _add_const(self, &wrap);
}

static size_t _add_iax(parser_t *self, candy_opcodes_t op, uint32_t a) {
  par_assert(a < (1U << 26), "instruction operand overflow");
  return candy_proto_add_inst(self->fs->proto, self->ls.gc, self->ls.ctx, (candy_inst_t) {
    .iax = {.op = (uint32_t)op, .a = a},
  });
}

static size_t _add_iabx(parser_t *self, candy_opcodes_t op, uint32_t a, uint32_t b) {
  par_assert(a < (1U << 8) && b < (1U << 18), "instruction operand overflow");
  return candy_proto_add_inst(self->fs->proto, self->ls.gc, self->ls.ctx, (candy_inst_t) {
    .iabx = {.op = (uint32_t)op, .a = a, .b = b},
  });
}

static size_t _add_iabc(parser_t *self, candy_opcodes_t op, uint32_t a, uint32_t b, uint32_t c) {
  par_assert(a < (1U << 8) && b < (1U << 9) && c < (1U << 9), "instruction operand overflow");
  return candy_proto_add_inst(self->fs->proto, self->ls.gc, self->ls.ctx, (candy_inst_t) {
    .iabc = {.op = (uint32_t)op, .a = a, .b = b, .c = c},
  });
}

static int _local(parser_t *self, const candy_array_t *name) {
  for (uint8_t i = 0; i < self->fs->nlocal; ++i)
    if (self->fs->locals[i] == name)
      return self->fs->local_reg[i];
  return self->fs->name == name ? 0 : -1;
}

static bool _is_local(parser_t *self, int reg) {
  for (uint8_t i = 0; i < self->fs->nlocal; ++i)
    if (self->fs->local_reg[i] == reg)
      return true;
  return false;
}

static int _declare_local(parser_t *self, const candy_array_t *name) {
  par_assert(self->fs->nlocal < UINT8_MAX, "too many locals");
  int reg = _newreg(self);
  self->fs->locals[self->fs->nlocal] = name;
  self->fs->local_reg[self->fs->nlocal++] = (uint8_t)reg;
  self->fs->local_hwm = self->fs->freereg;
  return reg;
}

static void _bind_name(parser_t *self, const candy_array_t *name, int reg) {
  if (!self->fs->prev) {
    _add_iabx(self, OP_SETG, (uint32_t)reg, (uint32_t)_add_name_const(self, name));
    return;
  }
  int local = _local(self, name);
  if (local <= 0)
    local = _declare_local(self, name);
  if (reg != local)
    _add_iabx(self, OP_MOVE, (uint32_t)local, (uint32_t)reg);
}

static void _check(parser_t *self, candy_tokens_t token) {
  par_assert(candy_lexer_lookahead(&self->ls) == token, "unexpected token %s, expected %s",
    candy_token_str(candy_lexer_lookahead(&self->ls)), candy_token_str(token));
}

static const candy_meta_t *_next(parser_t *self, candy_tokens_t token) {
  _check(self, token);
  return candy_lexer_next(&self->ls);
}

static bool _test(parser_t *self, candy_tokens_t token) {
  if (candy_lexer_lookahead(&self->ls) == token) {
    candy_lexer_next(&self->ls);
    return true;
  }
  return false;
}

static int _precedence(candy_tokens_t token) {
  switch (token) {
    case TK_OR: return 4;
    case TK_AND: return 5;
    case TK_LESS: case TK_GREATER: case TK_LEQUAL: case TK_GEQUAL: case TK_EQUAL: case TK_NEQUAL: return 10;
    case TK_AMPER: case TK_VERT: case TK_CARET: case TK_LSHIFT: case TK_RSHIFT: return 15;
    case TK_PLUS: case TK_MINUS: return 20;
    case TK_ASTE: case TK_SLASH: case TK_PERCENT: return 30;
    default: return -1;
  }
}

static uint32_t _call_args(parser_t *self, int base) {
  uint32_t narg = 0;
  if (!_test(self, ')')) {
    do {
      _expr(self, base + (int)narg, 0);
      ++narg;
    } while (_test(self, ','));
    _next(self, ')');
  }
  par_assert(narg <= UINT8_MAX, "too many arguments");
  return narg;
}

/* A name in expression position. A call returns the function's result register;
 * otherwise the value is returned in its own register. */
static int _name_expr(parser_t *self, const candy_array_t *name, int nres) {
  int local = _local(self, name);
  bool call = _test(self, '(');
  if (call) {
    /* The callee and arguments live at the top of the current register frame
     * so the callee's own frame starts above them (even for self calls). */
    int func = _newreg(self);
    if (local < 0)
      _add_iabx(self, OP_LOADG, (uint32_t)func, (uint32_t)_add_name_const(self, name));
    else if (local != 0)
      _add_iabx(self, OP_MOVE, (uint32_t)func, (uint32_t)local);
    uint32_t narg = _call_args(self, func + 1);
    _add_iabc(self, local == 0 ? OP_CALLSELF : OP_CALL, (uint32_t)func, narg, (uint32_t)nres);
    return func;
  }
  if (nres == 0)
    return -1;
  if (local >= 0)
    return local;
  int reg = _newreg(self);
  _add_iabx(self, OP_LOADG, (uint32_t)reg, (uint32_t)_add_name_const(self, name));
  return reg;
}

/* A table literal: either an array `{v1, v2, ...}` (integer keys 1..n) or a
 * JSON-style object `{"k": v, ...}` (string keys). */
static void _table_array_entry(parser_t *self, int table, size_t index, int val) {
  int key = _newreg(self);
  _add_iabx(self, OP_LOADK, (uint32_t)key, (uint32_t)_add_integer(self, (candy_integer_t)index));
  _add_iabc(self, OP_SETTABLE, (uint32_t)table, (uint32_t)key, (uint32_t)val);
  self->fs->freereg = (uint8_t)(table + 1);
}

static void _table_dict_entry(parser_t *self, int table, const candy_array_t *key) {
  int k = _newreg(self);
  _add_iabx(self, OP_LOADK, (uint32_t)k, (uint32_t)_add_name_const(self, key));
  int val = _expr(self, -1, 0);
  _add_iabc(self, OP_SETTABLE, (uint32_t)table, (uint32_t)k, (uint32_t)val);
  self->fs->freereg = (uint8_t)(table + 1);
}

static int _table_constructor(parser_t *self) {
  _next(self, '{');
  int table = _newreg(self);
  _add_iax(self, OP_TABLE, (uint32_t)table);
  if (candy_lexer_lookahead(&self->ls) == '}') {
    _next(self, '}');
    return table;
  }

  /* A leading string is either an object key or the first array element. */
  bool dict = false;
  size_t index = 0;
  if (candy_lexer_lookahead(&self->ls) == TK_STRING) {
    const candy_array_t *first = _next(self, TK_STRING)->s;
    if (candy_lexer_lookahead(&self->ls) == ':') {
      candy_lexer_next(&self->ls);
      _table_dict_entry(self, table, first);
      dict = true;
    }
    else {
      int val = _newreg(self);
      _add_iabx(self, OP_LOADK, (uint32_t)val, (uint32_t)_add_name_const(self, first));
      _table_array_entry(self, table, index++, val);
    }
  }
  else {
    int val = _expr(self, -1, 0);
    _table_array_entry(self, table, index++, val);
  }

  if (dict) {
    while (_test(self, ',')) {
      const candy_array_t *key = _next(self, TK_STRING)->s;
      _next(self, ':');
      _table_dict_entry(self, table, key);
    }
  }
  else {
    while (_test(self, ',')) {
      int val = _expr(self, -1, 0);
      _table_array_entry(self, table, index++, val);
    }
  }

  _next(self, '}');
  return table;
}

/* Postfix subscript access `t[k]`, reading each element into a fresh register. */
static int _postfix(parser_t *self, int reg) {
  while (candy_lexer_lookahead(&self->ls) == '[') {
    candy_lexer_next(&self->ls);
    int key = _expr(self, -1, 0);
    _next(self, ']');
    int dst = _newreg(self);
    _add_iabc(self, OP_GETTABLE, (uint32_t)dst, (uint32_t)reg, (uint32_t)key);
    reg = dst;
  }
  return reg;
}

static size_t _pc(parser_t *self) {
  return candy_vector_size(candy_proto_get_inst(self->fs->proto));
}

static void _patch_jmp(parser_t *self, size_t pos, size_t target) {
  par_assert(target < (1U << 26), "jump target overflow");
  candy_inst_t inst = ((const candy_inst_t *)candy_vector_data(candy_proto_get_inst(self->fs->proto)))[pos];
  inst.iax.a = (uint32_t)target;
  candy_proto_set_inst(self->fs->proto, pos, inst);
}

static void _patch_jmpf(parser_t *self, size_t pos, size_t target) {
  candy_proto_t *proto = self->fs->proto;
  candy_inst_t *data = (candy_inst_t *)candy_vector_data(candy_proto_get_inst(proto));
  candy_inst_t inst = data[pos];
  if (target < (1U << 18)) {
    inst.iabx.b = (uint32_t)target;
    candy_proto_set_inst(proto, pos, inst);
    return;
  }
  /* Target exceeds the 18-bit JMPF operand: rewrite it into a JMPT that skips
   * the following 26-bit JMP, shifting later instructions and their jump
   * targets by one. */
  par_assert(target < (1U << 26), "jump target overflow");
  size_t size = candy_vector_size(candy_proto_get_inst(proto));
  candy_proto_add_inst(proto, self->ls.gc, self->ls.ctx, inst);
  data = (candy_inst_t *)candy_vector_data(candy_proto_get_inst(proto));
  memmove(data + pos + 1, data + pos, (size - pos) * sizeof(candy_inst_t));
  data[pos] = (candy_inst_t) {.iabc = {.op = OP_JMPT, .a = inst.iabx.a}};
  data[pos + 1] = (candy_inst_t) {.iax = {.op = OP_JMP, .a = (uint32_t)target + 1}};
  for (size_t i = pos + 2; i <= size; ++i) {
    candy_inst_t *it = data + i;
    if (it->op == OP_JMP) {
      if (it->iax.a > pos)
        ++it->iax.a;
    }
    else if (it->op == OP_JMPF) {
      if (it->iabx.b > pos)
        ++it->iabx.b;
    }
  }
}

/* Patch either a JMPF (condition jump) or a plain JMP. */
static void _patch_jump(parser_t *self, size_t pos, size_t target) {
  candy_inst_t inst = ((const candy_inst_t *)candy_vector_data(candy_proto_get_inst(self->fs->proto)))[pos];
  if (inst.op == OP_JMPF)
    _patch_jmpf(self, pos, target);
  else
    _patch_jmp(self, pos, target);
}


/* Evaluate the tail of an arithmetic expression whose left value is in `lhs`.
 * `pref` is the preferred result register (or -1 for a scratch register). The
 * result lands in `pref` only when this is the whole expression's last step. */
static candy_opcodes_t _binary_op(parser_t *self, candy_tokens_t token, bool constant) {
  switch (token) {
    case TK_PLUS: return constant ? OP_ADDK : OP_ADD;
    case TK_MINUS: return constant ? OP_SUBK : OP_SUB;
    case TK_ASTE: return constant ? OP_MULK : OP_MUL;
    case TK_SLASH: return constant ? OP_DIVK : OP_DIV;
    case TK_PERCENT: return constant ? OP_MODK : OP_MOD;
    case TK_LESS: return constant ? OP_LTK : OP_LT;
    case TK_LEQUAL: return constant ? OP_LEK : OP_LE;
    case TK_GREATER: return constant ? OP_GTK : OP_GT;
    case TK_GEQUAL: return constant ? OP_GEK : OP_GE;
    case TK_EQUAL: return constant ? OP_EQK : OP_EQ;
    case TK_NEQUAL: return constant ? OP_NEQK : OP_NEQ;
    case TK_AMPER: return constant ? OP_BANDK : OP_BAND;
    case TK_VERT: return constant ? OP_BORK : OP_BOR;
    case TK_CARET: return constant ? OP_BXORK : OP_BXOR;
    case TK_LSHIFT: return constant ? OP_SHLK : OP_SHL;
    case TK_RSHIFT: return constant ? OP_SHRK : OP_SHR;
    default:
      par_assert(false, "unknown binary operator %s", candy_token_str(token));
      return OP_ADD;
  }
}

static int _expr_tail(parser_t *self, int lhs, int pref, int min_prec) {
  for (;;) {
    candy_tokens_t token = candy_lexer_lookahead(&self->ls);
    int prec = _precedence(token);
    if (prec < min_prec)
      break;
    candy_lexer_next(&self->ls);

    /* Short-circuit `&&`/`||`: the right operand is emitted after the jump so
     * it is only evaluated when needed. */
    if (token == TK_AND || token == TK_OR) {
      int result = _newreg(self);
      _add_iabc(self, OP_BOOL, (uint32_t)result, (uint32_t)lhs, 0);
      size_t skip = _add_iabx(self, OP_JMPF, (uint32_t)result, 0);
      size_t end = SIZE_MAX;
      if (token == TK_OR) {
        end = _add_iax(self, OP_JMP, 0);
        _patch_jump(self, skip, _pc(self));
      }
      int rhs = _expr(self, -1, prec + 1);
      _add_iabc(self, OP_BOOL, (uint32_t)result, (uint32_t)rhs, 0);
      if (token == TK_OR)
        _patch_jump(self, end, _pc(self));
      else
        _patch_jump(self, skip, _pc(self));
      lhs = result;
      continue;
    }

    int rhs_reg = -1;
    size_t rhs_const = 0;
    bool rhs_is_const = false;
    if (candy_lexer_lookahead(&self->ls) == TK_INTEGER) {
      size_t k = _add_meta(self, TK_INTEGER);
      if (_precedence(candy_lexer_lookahead(&self->ls)) <= prec) {
        /* A trailing constant folds into the operand; no higher-precedence
         * operator follows. A constant index beyond 9 bits is loaded first. */
        if (k < (1U << 9)) {
          rhs_is_const = true;
          rhs_const = k;
        }
        else {
          rhs_reg = _newreg(self);
          _add_iabx(self, OP_LOADK, (uint32_t)rhs_reg, (uint32_t)k);
        }
      }
      else {
        /* A higher-precedence operator follows: the constant is only the head
         * of the right operand (e.g. `4 < 2 + 1`). */
        int c = _newreg(self);
        _add_iabx(self, OP_LOADK, (uint32_t)c, (uint32_t)k);
        rhs_reg = _expr_tail(self, c, -1, prec + 1);
      }
    }
    else {
      rhs_reg = _expr(self, -1, prec + 1);
    }

    bool last = _precedence(candy_lexer_lookahead(&self->ls)) < min_prec;
    bool dead = lhs == (int)self->fs->freereg - 1 && !_is_local(self, lhs);
    int result;
    if (last && pref >= 0)
      result = pref;
    else if (dead)
      result = lhs;
    else
      result = _newreg(self);

    candy_opcodes_t op = _binary_op(self, token, rhs_is_const);
    _add_iabc(self, op, (uint32_t)result, (uint32_t)lhs,
      rhs_is_const ? (uint32_t)rhs_const : (uint32_t)rhs_reg);
    lhs = result;
  }
  return lhs;
}

/* A `[v1, v2, ...]` array literal: identical to `{v1, v2, ...}` but with the
 * JSON bracket spelling. */
static int _array_constructor(parser_t *self) {
  _next(self, '[');
  int array = _newreg(self);
  _add_iax(self, OP_ARRAY, (uint32_t)array);
  if (candy_lexer_lookahead(&self->ls) == ']') {
    _next(self, ']');
    return array;
  }

  /* Collect literal elements into the constant pool; a fully-literal list is
   * filled with one SETLIST instead of one SETTABLE per element. */
  size_t count = 0;
  size_t start = 0;
  size_t pending = SIZE_MAX;
  for (;;) {
    candy_tokens_t token = candy_lexer_lookahead(&self->ls);
    if (!_is_literal(token))
      break;
    candy_wrap_t wrap = _literal_wrap(self, token);
    size_t pos = candy_proto_add_const_unique(self->fs->proto, self->ls.gc, self->ls.ctx, &wrap);
    candy_tokens_t next = candy_lexer_lookahead(&self->ls);
    if (next != ',' && next != ']') {
      pending = pos;
      break;
    }
    if (count == 0)
      start = pos;
    ++count;
    if (!_test(self, ',')) {
      int count_reg = _newreg(self);
      _add_iabx(self, OP_LOADK, (uint32_t)count_reg, (uint32_t)_add_integer(self, (candy_integer_t)count));
      int start_reg = _newreg(self);
      _add_iabx(self, OP_LOADK, (uint32_t)start_reg, (uint32_t)_add_integer(self, (candy_integer_t)start));
      _add_iabc(self, OP_SETLIST, (uint32_t)array, (uint32_t)count_reg, (uint32_t)start_reg);
      _next(self, ']');
      return array;
    }
  }

  /* A non-literal element appeared: emit the collected constants, then fall
   * back to per-element SETTABLE for the rest. */
  size_t index = 0;
  for (size_t i = 0; i < count; ++i) {
    int val = _newreg(self);
    _add_iabx(self, OP_LOADK, (uint32_t)val, (uint32_t)(start + i));
    _table_array_entry(self, array, index++, val);
  }
  for (;;) {
    int val;
    if (pending != SIZE_MAX) {
      val = _newreg(self);
      _add_iabx(self, OP_LOADK, (uint32_t)val, (uint32_t)pending);
      val = _expr_tail(self, _postfix(self, val), -1, 0);
      pending = SIZE_MAX;
    }
    else {
      val = _expr(self, -1, 0);
    }
    _table_array_entry(self, array, index++, val);
    if (!_test(self, ','))
      break;
  }
  _next(self, ']');
  return array;
}

static int _primary(parser_t *self) {
  candy_tokens_t token = candy_lexer_lookahead(&self->ls);
  if (token == TK_MINUS) {
    /* unary minus `-x` lowers to `0 - x` */
    candy_lexer_next(&self->ls);
    int operand = _expr(self, -1, 30);
    int zero = _newreg(self);
    _add_iabx(self, OP_LOADK, (uint32_t)zero, (uint32_t)_add_integer(self, 0));
    int reg = _newreg(self);
    _add_iabc(self, OP_SUB, (uint32_t)reg, (uint32_t)zero, (uint32_t)operand);
    return reg;
  }
  if (token == TK_EXCLA) {
    candy_lexer_next(&self->ls);
    int operand = _expr(self, -1, 30);
    int reg = _newreg(self);
    _add_iabc(self, OP_NOT, (uint32_t)reg, (uint32_t)operand, 0);
    return reg;
  }
  if (token == '(') {
    candy_lexer_next(&self->ls);
    int reg = _expr(self, -1, 0);
    _next(self, ')');
    return reg;
  }
  if (token == '{') {
    return _table_constructor(self);
  }
  if (token == '[') {
    return _array_constructor(self);
  }
  if (token == TK_IDENT) {
    const candy_array_t *name = _next(self, TK_IDENT)->s;
    return _name_expr(self, name, 1);
  }
  par_assert(token == TK_INTEGER || token == TK_FLOAT || token == TK_STRING ||
    token == TK_true || token == TK_false || token == TK_null,
    "unexpected token %s in expression", candy_token_str(token));
  int reg = _newreg(self);
  _add_iabx(self, OP_LOADK, (uint32_t)reg, (uint32_t)_add_meta(self, token));
  return reg;
}

static int _expr(parser_t *self, int pref, int min_prec) {
  int lhs = _postfix(self, _primary(self));
  int result = _expr_tail(self, lhs, pref, min_prec);
  if (pref >= 0) {
    if (result != pref)
      _add_iabx(self, OP_MOVE, (uint32_t)pref, (uint32_t)result);
    /* A call slot or parameter register may sit above the current scratch
     * high-water mark; account for it so the frame is large enough. */
    if ((int)self->fs->freereg <= pref) {
      self->fs->freereg = (uint8_t)(pref + 1);
      if (self->fs->freereg > self->fs->nreg)
        self->fs->nreg = self->fs->freereg;
    }
  }
  return result;
}

/* Pending jumps form a list in their own operand fields: zero ends the list,
 * otherwise the operand is the next instruction index plus one. */
static void _link_jump(parser_t *self, size_t *head) {
  size_t pos = _pc(self);
  par_assert(pos + 1 < (1U << 26) && *head < (1U << 26), "too many jumps");
  _add_iax(self, OP_JMP, (uint32_t)*head);
  *head = pos + 1;
}

static void _patch_list(parser_t *self, size_t head, size_t target) {
  while (head) {
    size_t pos = head - 1;
    const candy_inst_t *code = (const candy_inst_t *)candy_vector_data(
      candy_proto_get_inst(self->fs->proto));
    head = code[pos].iax.a;
    _patch_jmp(self, pos, target);
  }
}

/* Parse a parenthesized condition and emit a jump taken when it is false.
 * Returns the JMPF instruction index for later patching. */
static size_t _condition(parser_t *self) {
  _next(self, '(');
  int reg = _expr(self, -1, 0);
  _next(self, ')');
  return _add_iabx(self, OP_JMPF, (uint32_t)reg, 0);
}

static bool _compound_op(parser_t *self, candy_tokens_t token, candy_opcodes_t *op) {
  switch (token) {
    case TK_ADDASS: token = TK_PLUS; break;
    case TK_SUBASS: token = TK_MINUS; break;
    case TK_MULASS: token = TK_ASTE; break;
    case TK_DIVASS: token = TK_SLASH; break;
    case TK_MODASS: token = TK_PERCENT; break;
    case TK_AMPASS: token = TK_AMPER; break;
    case TK_VERASS: token = TK_VERT; break;
    case TK_CARASS: token = TK_CARET; break;
    default: return false;
  }
  *op = _binary_op(self, token, false);
  return true;
}

/* Store an in-progress `x + ...` value, continuing any remaining operators. */
static bool _finish_global_add(parser_t *self, const candy_array_t *name, size_t key, int sum) {
  (void)name;
  int result = _expr_tail(self, sum, -1, 0);
  if (result != sum)
    _add_iabx(self, OP_SETG, (uint32_t)result, (uint32_t)key);
  else
    _add_iabx(self, OP_SETG, (uint32_t)sum, (uint32_t)key);
  return true;
}

/* `x = x + <simple>` in the main chunk: read-modify-write in one instruction.
 * Only a bare name or an integer literal is fused, because those operands have
 * no side effects; every other shape keeps the original evaluation order. */
static bool _fuse_global_add(parser_t *self, const candy_array_t *name) {
  size_t key = _add_name_const(self, name);
  if (key >= (1U << 9))
    return false;
  candy_tokens_t token = candy_lexer_lookahead(&self->ls);
  if (token == TK_INTEGER) {
    size_t constant = _add_meta(self, TK_INTEGER);
    if (_precedence(candy_lexer_lookahead(&self->ls)) < 0 && constant < (1U << 9)) {
      _add_iabc(self, OP_ADDGK, 0, (uint32_t)key, (uint32_t)constant);
      return true;
    }
    int current = _newreg(self);
    _add_iabx(self, OP_LOADG, (uint32_t)current, (uint32_t)key);
    int operand = _newreg(self);
    _add_iabx(self, OP_LOADK, (uint32_t)operand, (uint32_t)constant);
    operand = _expr_tail(self, operand, -1, 21);
    int sum = _newreg(self);
    _add_iabc(self, OP_ADD, (uint32_t)sum, (uint32_t)current, (uint32_t)operand);
    return _finish_global_add(self, name, key, sum);
  }
  if (token != TK_IDENT)
    return false;
  const candy_array_t *operand_name = _next(self, TK_IDENT)->s;
  candy_tokens_t after = candy_lexer_lookahead(&self->ls);
  if (after != '(' && after != '[' && _precedence(after) < 0) {
    int operand = _name_expr(self, operand_name, 1);
    if ((uint32_t)operand < (1U << 9)) {
      _add_iabc(self, OP_ADDG, 0, (uint32_t)key, (uint32_t)operand);
      return true;
    }
  }
  /* Complex operand: load the global first, exactly like the generic path. */
  int current = _newreg(self);
  _add_iabx(self, OP_LOADG, (uint32_t)current, (uint32_t)key);
  int operand = _expr_tail(self, _postfix(self, _name_expr(self, operand_name, 1)), -1, 21);
  int sum = _newreg(self);
  _add_iabc(self, OP_ADD, (uint32_t)sum, (uint32_t)current, (uint32_t)operand);
  return _finish_global_add(self, name, key, sum);
}

static void _name_stmt(parser_t *self) {
  const candy_array_t *name = _next(self, TK_IDENT)->s;
  if (candy_lexer_lookahead(&self->ls) == '[') {
    /* subscript assignment `t[k] = v` or `t[k] op= v` */
    int table = _name_expr(self, name, 1);
    candy_lexer_next(&self->ls);
    int key = _expr(self, -1, 0);
    _next(self, ']');
    candy_opcodes_t op;
    if (_compound_op(self, candy_lexer_lookahead(&self->ls), &op)) {
      candy_lexer_next(&self->ls);
      int val = _expr(self, -1, 0);
      int t = _newreg(self);
      _add_iabc(self, OP_GETTABLE, (uint32_t)t, (uint32_t)table, (uint32_t)key);
      _add_iabc(self, op, (uint32_t)t, (uint32_t)t, (uint32_t)val);
      _add_iabc(self, OP_SETTABLE, (uint32_t)table, (uint32_t)key, (uint32_t)t);
      return;
    }
    _next(self, '=');
    int val = _expr(self, -1, 0);
    _add_iabc(self, OP_SETTABLE, (uint32_t)table, (uint32_t)key, (uint32_t)val);
    return;
  }
  candy_opcodes_t op;
  if (_compound_op(self, candy_lexer_lookahead(&self->ls), &op)) {
    candy_lexer_next(&self->ls);
    int target = _name_expr(self, name, 1);
    int val = _expr(self, -1, 0);
    _add_iabc(self, op, (uint32_t)target, (uint32_t)target, (uint32_t)val);
    if (_local(self, name) < 0)
      _add_iabx(self, OP_SETG, (uint32_t)target, (uint32_t)_add_name_const(self, name));
    return;
  }
  if (!_test(self, '=')) {
    _name_expr(self, name, 0);
    return;
  }
  int local = _local(self, name);
  if (local > 0) {
    _expr(self, local, 0);
    return;
  }
  /* Only the main chunk assigns globals directly; inside a function the first
   * assignment must bind a local, so fusion there would change the binding. */
  if (local < 0 && !self->fs->prev && candy_lexer_lookahead(&self->ls) == TK_IDENT) {
    const candy_array_t *first = _next(self, TK_IDENT)->s;
    if (first == name && _test(self, '+')) {
      if (_fuse_global_add(self, name))
        return;
      /* Not fused: we consumed `x +`, so parse the right operand from here. */
      int current = _name_expr(self, first, 1);
      int operand = _expr(self, -1, 21);
      int sum = _newreg(self);
      _add_iabc(self, OP_ADD, (uint32_t)sum, (uint32_t)current, (uint32_t)operand);
      _bind_name(self, name, _expr_tail(self, sum, -1, 0));
      return;
    }
    /* Not fused: the identifier we consumed is the head of the right operand. */
    _bind_name(self, name, _expr_tail(self, _postfix(self, _name_expr(self, first, 1)), -1, 0));
    return;
  }
  int reg = _expr(self, -1, 0);
  _bind_name(self, name, reg);
}

static void _def_stmt(parser_t *self) {
  _next(self, TK_def);
  const candy_array_t *name = _next(self, TK_IDENT)->s;
  funcstate_t fs;
  _funcstate_open(&fs, self);
  fs.name = name;
  _next(self, '(');
  if (!_test(self, ')')) {
    do {
      const candy_array_t *param = _next(self, TK_IDENT)->s;
      par_assert(_local(self, param) <= 0, "duplicate parameter");
      _declare_local(self, param);
    } while (_test(self, ','));
    _next(self, ')');
  }
  uint32_t nparams = fs.nlocal;
  fs.local_hwm = fs.freereg;
  bool returns = _block(self, true);
  _next(self, TK_end);
  /* A body that ends in `return` cannot fall through, so it needs no trailing
   * return (and no extra register for one). */
  if (!returns) {
    int null = fs.local_hwm > 1 ? (int)fs.local_hwm - 1 : _newreg(self);
    _add_iabx(self, OP_LOADK, (uint32_t)null, (uint32_t)_add_null(self));
    _add_iax(self, OP_RETURN, (uint32_t)null);
  }
  candy_proto_set_layout(fs.proto, (uint8_t)nparams, fs.nreg, fs.local_hwm);

  candy_proto_t *proto = fs.proto;
  _funcstate_close(&fs, self);
  candy_sclosure_t *closure = candy_sclosure_create(self->ls.gc, self->ls.ctx, proto);
  candy_wrap_t wrap;
  candy_wrap_set_object(&wrap, (candy_object_t *)closure);
  int reg = _newreg(self);
  _add_iabx(self, OP_LOADK, (uint32_t)reg, (uint32_t)_add_const(self, &wrap));
  _bind_name(self, name, reg);
}

static void _if_stmt(parser_t *self) {
  _next(self, TK_if);
  size_t exits = 0;
  for (;;) {
    size_t fail = _condition(self);
    _block(self, true);
    candy_tokens_t token = candy_lexer_lookahead(&self->ls);
    if (token == TK_elif || token == TK_else)
      _link_jump(self, &exits);
    _patch_jump(self, fail, _pc(self));
    if (_test(self, TK_elif))
      continue;
    if (_test(self, TK_else))
      _block(self, true);
    _next(self, TK_end);
    break;
  }
  _patch_list(self, exits, _pc(self));
}

static void _loop_close(parser_t *self, loopstate_t *loop) {
  _patch_list(self, loop->breaks, _pc(self));
  self->fs->loop = loop->prev;
}

static void _while_stmt(parser_t *self) {
  _next(self, TK_while);
  size_t condition = _pc(self);
  size_t fail = _condition(self);
  /* Locals declared in the body must not reuse the condition's registers, or
   * reading them after the loop would see the last condition result. */
  self->fs->local_hwm = self->fs->freereg;
  loopstate_t loop = {self->fs->loop, condition, 0, 0};
  self->fs->loop = &loop;
  (void)_block(self, true);
  _next(self, TK_end);
  _add_iax(self, OP_JMP, (uint32_t)condition);
  _patch_jump(self, fail, _pc(self));
  _loop_close(self, &loop);
}

static void _for_stmt(parser_t *self) {
  _next(self, TK_for);
  _next(self, '(');
  const candy_array_t *var = _next(self, TK_IDENT)->s;
  _next(self, ':');
  int iter = _expr(self, -1, 0);
  _next(self, ')');

  bool global = !self->fs->prev;
  int slot = global ? _newreg(self) : _local(self, var);
  if (slot <= 0)
    slot = _declare_local(self, var);

  int idx = _newreg(self);
  int len = _newreg(self);
  _add_iabx(self, OP_LOADK, (uint32_t)idx, (uint32_t)_add_integer(self, 0));
  _add_iabx(self, OP_LEN, (uint32_t)len, (uint32_t)iter);
  int cmp = _newreg(self);
  self->fs->local_hwm = (uint8_t)(cmp + 1);

  _add_iabc(self, OP_LT, (uint32_t)cmp, (uint32_t)idx, (uint32_t)len);
  size_t fail = _add_iabx(self, OP_JMPF, (uint32_t)cmp, 0);
  size_t body = _pc(self);
  _add_iabc(self, OP_GETTABLE, (uint32_t)slot, (uint32_t)iter, (uint32_t)idx);
  if (global)
    _add_iabx(self, OP_SETG, (uint32_t)slot, (uint32_t)_add_name_const(self, var));

  loopstate_t loop = {self->fs->loop, SIZE_MAX, 0, 0};
  self->fs->loop = &loop;
  (void)_block(self, true);
  _next(self, TK_end);
  loop.continue_pc = _pc(self);
  _patch_list(self, loop.continues, loop.continue_pc);

  _add_iabx(self, OP_FORLOOP, (uint32_t)idx, (uint32_t)body);
  _patch_jump(self, fail, _pc(self));
  _loop_close(self, &loop);
}

static void _break_stmt(parser_t *self) {
  _next(self, TK_break);
  par_assert(self->fs->loop != NULL, "break outside loop");
  _link_jump(self, &self->fs->loop->breaks);
}

static void _continue_stmt(parser_t *self) {
  _next(self, TK_continue);
  par_assert(self->fs->loop != NULL, "continue outside loop");
  if (self->fs->loop->continue_pc == SIZE_MAX)
    _link_jump(self, &self->fs->loop->continues);
  else
    _add_iax(self, OP_JMP, (uint32_t)self->fs->loop->continue_pc);
}

static void _return_stmt(parser_t *self) {
  _next(self, TK_return);
  size_t before = _pc(self);
  int reg = _expr(self, -1, 0);
  size_t after = _pc(self);
  if (after > before) {
    const candy_inst_t *code = (const candy_inst_t *)candy_vector_data(
      candy_proto_get_inst(self->fs->proto));
    candy_inst_t last = code[after - 1];
    if (last.op == OP_CALL && last.iabc.c == 1) {
      candy_proto_set_inst(self->fs->proto, after - 1, (candy_inst_t) {
        .iabx = {.op = OP_TAILCALL, .a = last.iabc.a, .b = last.iabc.b},
      });
      return;
    }
    if (last.op == OP_CALLSELF && last.iabc.c == 1) {
      candy_proto_set_inst(self->fs->proto, after - 1, (candy_inst_t) {
        .iabx = {.op = OP_TAILCALLSELF, .a = last.iabc.a, .b = last.iabc.b},
      });
      return;
    }
  }
  _add_iax(self, OP_RETURN, (uint32_t)reg);
}

/* Whether the block's last statement is `return`; a body that ends with one
 * cannot fall through, so callers can skip a trailing return of their own. */
static bool _block(parser_t *self, bool stop_at_end) {
  bool returns = false;
  for (candy_tokens_t token = candy_lexer_lookahead(&self->ls); ; token = candy_lexer_lookahead(&self->ls)) {
    self->fs->freereg = self->fs->local_hwm;
    switch (token) {
      case TK_EOS:
        return returns;
      case TK_end: case TK_elif: case TK_else:
        par_assert(stop_at_end, "unexpected token %s in statement", candy_token_str(token));
        return returns;
      case TK_def:      _def_stmt(self); break;
      case TK_if:       _if_stmt(self); break;
      case TK_while:    _while_stmt(self); break;
      case TK_for:      _for_stmt(self); break;
      case TK_break:    _break_stmt(self); break;
      case TK_continue: _continue_stmt(self); break;
      case TK_return:   _return_stmt(self); break;
      case TK_IDENT:    _name_stmt(self); break;
      default:
        par_assert(false, "unexpected token %s in statement", candy_token_str(token));
    }
    returns = token == TK_return;
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

static void _entry(void *arg) {
  parser_t *self = (parser_t *)arg;
  bool returns = _block(self, false);
  _check(self, TK_EOS);
  if (!returns) {
    int null = self->fs->local_hwm > 1 ? (int)self->fs->local_hwm - 1 : _newreg(self);
    _add_iabx(self, OP_LOADK, (uint32_t)null, (uint32_t)_add_null(self));
    _add_iax(self, OP_RETURN, (uint32_t)null);
  }
  candy_proto_set_layout(self->fs->proto, 0, self->fs->nreg, self->fs->local_hwm);
}

candy_err_t candy_parse(candy_gc_t *gc, candy_excep_t *ctx, candy_reader_t reader, void *arg, candy_object_t **out) {
  parser_t prsr;
  funcstate_t fs;
  _parser_init(&prsr, gc, ctx, reader, arg);
  _funcstate_open(&fs, &prsr);
  candy_err_t err = candy_excep_try(ctx, _entry, &prsr, out);
  _funcstate_close(&fs, &prsr);
  _parser_deinit(&prsr);
  if (err == CANDY_OK)
    *out = (candy_object_t *)candy_sclosure_create(gc, ctx, fs.proto);
  return err;
}
