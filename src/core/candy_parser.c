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
#include "core/candy_table.h"
#include "core/candy_lexer.h"
#include "core/candy_reader.h"
#include <string.h>

#define par_assert(_condition, _format, ...) \
candy_assert(self->ls.ctx, self->ls.gc, _condition, CANDY_ERR_SYNTAX, \
"line %zu col %zu: " _format, self->ls.dbg.line, self->ls.dbg.column, ##__VA_ARGS__)

#define CANDY_ABX_LIMIT (1U << 18)

typedef struct loopstate loopstate_t;
typedef struct funcstate funcstate_t;
typedef struct parser parser_t;

struct loopstate {
  loopstate_t *prev;
  size_t continue_pc;
  size_t breaks;
  size_t continues;
};

struct parser_local { const candy_array_t *name; uint8_t reg; };
struct parser_capture { const candy_array_t *name; int8_t mode; };

struct funcstate {
  funcstate_t *prev;
  candy_proto_t *proto;
  const candy_array_t *name;
  candy_vector_t locals;
  candy_vector_t captures;
  candy_vector_t upvals;
  uint8_t nlocal;
  uint8_t freereg;
  uint8_t local_hwm;
  uint8_t nreg;
  loopstate_t *loop;
  bool lambda;
  int8_t capture_default;
  uint8_t ncaptures;
  uint8_t nupvals;
};

struct parse_graph {
  candy_array_t *plan;
  candy_array_t *pending;
  candy_array_t *aliases;
  candy_table_t *compiled;
};

struct parser {
  candy_lexer_t ls;
  funcstate_t *fs;
  candy_table_t *globals;
  candy_table_t *exports;
  const char *path;
  const parser_t *parent;
  struct parse_graph *graph;
};

struct parse_module {
  parser_t parser;
  candy_object_t *entry;
};

static int _expr(parser_t *self, int pref, int min_prec);
static bool _block(parser_t *self, bool stop_at_end);

static void _funcstate_open(funcstate_t *self, parser_t *prsr) {
  self->prev = prsr->fs;
  self->proto = candy_proto_create(prsr->ls.gc, prsr->ls.ctx);
  candy_proto_set_globals(self->proto, prsr->globals);
  self->name = NULL;
  candy_vector_init(&self->locals);
  candy_vector_init(&self->captures);
  candy_vector_init(&self->upvals);
  self->nlocal = 0;
  self->freereg = 1;
  self->local_hwm = 1;
  self->nreg = 1;
  self->loop = NULL;
  self->lambda = false;
  self->capture_default = -1;
  self->ncaptures = 0;
  self->nupvals = 0;
  prsr->fs = self;
}

static void _funcstate_close(funcstate_t *self, parser_t *prsr) {
  if (candy_vector_data(&self->locals))
    candy_vector_deinit(&self->locals, candy_gc_memory(prsr->ls.gc), sizeof(struct parser_local));
  if (candy_vector_data(&self->captures))
    candy_vector_deinit(&self->captures, candy_gc_memory(prsr->ls.gc), sizeof(struct parser_capture));
  if (candy_vector_data(&self->upvals))
    candy_vector_deinit(&self->upvals, candy_gc_memory(prsr->ls.gc), sizeof(const candy_array_t *));
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
  par_assert(a < (1U << 8) && b < CANDY_ABX_LIMIT, "instruction operand overflow");
  return candy_proto_add_inst(self->fs->proto, self->ls.gc, self->ls.ctx, (candy_inst_t) {
    .iabx = {.op = (uint32_t)op, .a = a, .b = b},
  });
}

static void _load_integer(parser_t *self, uint32_t reg, candy_integer_t value) {
  candy_wrap_t wrap;
  if (value >= 0 && value < CANDY_ABX_LIMIT) {
    _add_iabx(self, OP_LOADI, reg, (uint32_t)value);
    return;
  }
  candy_wrap_set_integer(&wrap, value);
  _add_iabx(self, OP_LOADK, reg, (uint32_t)_add_const(self, &wrap));
}

static size_t _add_iabc(parser_t *self, candy_opcodes_t op, uint32_t a, uint32_t b, uint32_t c) {
  par_assert(a < (1U << 8) && b < (1U << 9) && c < (1U << 9), "instruction operand overflow");
  return candy_proto_add_inst(self->fs->proto, self->ls.gc, self->ls.ctx, (candy_inst_t) {
    .iabc = {.op = (uint32_t)op, .a = a, .b = b, .c = c},
  });
}

static int _local(parser_t *self, const candy_array_t *name) {
  for (uint8_t i = 0; i < self->fs->nlocal; ++i)
    if (((struct parser_local *)candy_vector_data(&self->fs->locals))[i].name == name)
      return ((struct parser_local *)candy_vector_data(&self->fs->locals))[i].reg;
  return self->fs->name == name ? 0 : -1;
}

static bool _is_local(parser_t *self, int reg) {
  for (uint8_t i = 0; i < self->fs->nlocal; ++i)
    if (((struct parser_local *)candy_vector_data(&self->fs->locals))[i].reg == reg)
      return true;
  return false;
}

static int _declare_local(parser_t *self, const candy_array_t *name) {
  par_assert(self->fs->nlocal < UINT8_MAX, "too many locals");
  int reg = _newreg(self);
  candy_vector_append(&self->fs->locals, self->ls.gc ? candy_gc_memory(self->ls.gc) : NULL, self->ls.ctx, &(struct parser_local){name, (uint8_t)reg}, 1, sizeof(struct parser_local));
  self->fs->nlocal++;
  self->fs->local_hwm = self->fs->freereg;
  return reg;
}

/* Does `name` name a local (or the function itself) at `fs` or above? */
static bool _scope_has(parser_t *self, funcstate_t *fs, const candy_array_t *name) {
  (void)self;
  for (; fs != NULL; fs = fs->prev) {
    if (fs->name == name)
      return true;
    for (uint8_t i = 0; i < fs->nlocal; ++i)
      if (((struct parser_local *)candy_vector_data(&fs->locals))[i].name == name)
        return true;
    for (uint8_t i = 0; i < fs->nupvals; ++i)
      if (((const candy_array_t **)candy_vector_data(&fs->upvals))[i] == name)
        return true;
  }
  return false;
}

/* Register an upvalue of `fs` for `name`, captured from its parent frame.
 * Intermediate functions capture on demand (always by reference) so a nested
 * lambda can reach variables of its grand-parents. */
static int _upvalue_ensure(parser_t *self, funcstate_t *fs, const candy_array_t *name, bool by_ref) {
  for (uint8_t i = 0; i < fs->nupvals; ++i)
    if (((const candy_array_t **)candy_vector_data(&fs->upvals))[i] == name)
      return i;
  par_assert(fs->nupvals < UINT8_MAX, "too many upvalues");
  funcstate_t *parent = fs->prev;
  par_assert(parent != NULL, "upvalue without an enclosing function");
  int reg = -1;
  for (uint8_t i = 0; i < parent->nlocal; ++i)
    if (((struct parser_local *)candy_vector_data(&parent->locals))[i].name == name) {
      reg = ((struct parser_local *)candy_vector_data(&parent->locals))[i].reg;
      break;
    }
  if (reg < 0 && parent->name == name)
    reg = 0;
  int from = -1;
  if (reg < 0) {
    for (uint8_t i = 0; i < parent->nupvals; ++i)
      if (((const candy_array_t **)candy_vector_data(&parent->upvals))[i] == name) {
        from = i;
        break;
      }
    if (from < 0) {
      if (!_scope_has(self, parent, name))
        return -1;
      from = _upvalue_ensure(self, parent, name, true);
      if (from < 0)
        return -1;
    }
  }
  uint8_t kind = reg >= 0 ? (by_ref ? 0 : 2) : (by_ref ? 1 : 3);
  uint8_t index = (uint8_t)(reg >= 0 ? reg : from);
  candy_proto_add_upval(fs->proto, self->ls.gc, self->ls.ctx, kind, index);
  candy_vector_append(&fs->upvals, candy_gc_memory(self->ls.gc), self->ls.ctx, &name, 1, sizeof(name));
  return fs->nupvals++;
}

/* Resolve `name` as an upvalue of the current lambda: -1 when it is not a
 * captured variable (globals and named functions fall through), otherwise the
 * upvalue index, creating the capture when needed. */
static int _upvalue_resolve(parser_t *self, const candy_array_t *name) {
  if (!self->fs->lambda)
    return -1;
  for (uint8_t i = 0; i < self->fs->nupvals; ++i)
    if (((const candy_array_t **)candy_vector_data(&self->fs->upvals))[i] == name)
      return i;
  bool named = false;
  int8_t mode = self->fs->capture_default;
  for (uint8_t i = 0; i < self->fs->ncaptures; ++i)
    if (((struct parser_capture *)candy_vector_data(&self->fs->captures))[i].name == name) {
      mode = ((struct parser_capture *)candy_vector_data(&self->fs->captures))[i].mode;
      named = true;
      break;
    }
  if (mode < 0) {
    /* C++ rule: an enclosing variable used by a lambda must be captured. */
    par_assert(!_scope_has(self, self->fs->prev, name), "name is not captured: %.*s",
      (int)candy_array_size(name), (char *)candy_array_data(name));
    return -1;
  }
  int index = _upvalue_ensure(self, self->fs, name, mode == 1);
  /* A default capture only applies to variables that are actually in scope. */
  par_assert(!named || index >= 0, "cannot capture: %.*s",
    (int)candy_array_size(name), (char *)candy_array_data(name));
  return index;
}

static void _bind_name(parser_t *self, const candy_array_t *name, int reg) {
  if (!self->fs->prev) {
    _add_iabx(self, OP_SETG, (uint32_t)reg, (uint32_t)_add_name_const(self, name));
    return;
  }
  int local = _local(self, name);
  if (local <= 0) {
    int upval = self->fs->lambda ? _upvalue_resolve(self, name) : -1;
    if (upval >= 0) {
      _add_iabx(self, OP_SETUPVAL, (uint32_t)reg, (uint32_t)upval);
      return;
    }
    local = _declare_local(self, name);
  }
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
  int upval = local < 0 && self->fs->lambda ? _upvalue_resolve(self, name) : -1;
  bool call = _test(self, '(');
  if (call) {
    /* The callee and arguments live at the top of the current register frame
     * so the callee's own frame starts above them (even for self calls). */
    int func = _newreg(self);
    if (local < 0 && upval >= 0)
      _add_iabx(self, OP_GETUPVAL, (uint32_t)func, (uint32_t)upval);
    else if (local < 0)
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
  if (upval >= 0)
    _add_iabx(self, OP_GETUPVAL, (uint32_t)reg, (uint32_t)upval);
  else
    _add_iabx(self, OP_LOADG, (uint32_t)reg, (uint32_t)_add_name_const(self, name));
  return reg;
}

/* A table literal: either an array `{v1, v2, ...}` (integer keys 1..n) or a
 * JSON-style object `{"k": v, ...}` (string keys). */
static void _table_array_entry(parser_t *self, int table, size_t index, int val) {
  int key = _newreg(self);
  _load_integer(self, (uint32_t)key, (candy_integer_t)index);
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
  _add_iabx(self, OP_NEWCONTAINER, (uint32_t)table, 0);
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
/* Call the value already in `reg`; the opening parenthesis is consumed. The
 * callee is moved to the top of the frame so its arguments follow it. */
static int _call_value(parser_t *self, int reg) {
  if (reg != (int)self->fs->freereg - 1) {
    int moved = _newreg(self);
    _add_iabx(self, OP_MOVE, (uint32_t)moved, (uint32_t)reg);
    reg = moved;
  }
  uint32_t narg = _call_args(self, reg + 1);
  _add_iabc(self, OP_CALL, (uint32_t)reg, narg, 1);
  return reg;
}

/* Subscripts and calls chain left to right, so `a[0](x)` and `f()()` both work
 * on any expression, not just on names. */
static int _call_method(parser_t *self, int receiver, int key) {
  int callee = _newreg(self);
  _add_iabc(self, OP_GETTABLE, (uint32_t)callee, (uint32_t)receiver, (uint32_t)key);
  int slot = _newreg(self);
  _add_iabx(self, OP_MOVE, (uint32_t)slot, (uint32_t)receiver);
  uint32_t narg = _call_args(self, slot + 1);
  _add_iabc(self, OP_CALLMETHOD, (uint32_t)callee, narg, 1);
  return callee;
}

static int _postfix(parser_t *self, int reg) {
  for (;;) {
    candy_tokens_t token = candy_lexer_lookahead(&self->ls);
    if (token == '[') {
      candy_lexer_next(&self->ls);
      int key = _expr(self, -1, 0);
      _next(self, ']');
      int dst = _newreg(self);
      _add_iabc(self, OP_GETTABLE, (uint32_t)dst, (uint32_t)reg, (uint32_t)key);
      reg = dst;
      continue;
    }
    if (token == TK_DOT || token == TK_SCOPE) {
      candy_lexer_next(&self->ls);
      const candy_array_t *field = _next(self, TK_IDENT)->s;
      int key = _newreg(self);
      _add_iabx(self, OP_LOADK, (uint32_t)key, (uint32_t)_add_name_const(self, field));
      if (token == TK_DOT && candy_lexer_lookahead(&self->ls) == '(') {
        candy_lexer_next(&self->ls);
        reg = _call_method(self, reg, key);
      }
      else {
        int dst = _newreg(self);
        _add_iabc(self, OP_GETTABLE, (uint32_t)dst, (uint32_t)reg, (uint32_t)key);
        reg = dst;
      }
      continue;
    }
    if (token == '(') {
      candy_lexer_next(&self->ls);
      reg = _call_value(self, reg);
      continue;
    }
    return reg;
  }
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
  if (target < CANDY_ABX_LIMIT) {
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
    case TK_GREATER: return constant ? OP_GTK : OP_LT;
    case TK_GEQUAL: return constant ? OP_GEK : OP_LE;
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
    uint32_t rhs;
    bool rhs_is_const = false;
    bool rhs_is_immediate = false;
    int result;
    candy_opcodes_t op;
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

    if (candy_lexer_lookahead(&self->ls) == TK_INTEGER) {
      candy_integer_t value = candy_lexer_next(&self->ls)->i;
      bool trailing = _precedence(candy_lexer_lookahead(&self->ls)) <= prec;
      /* Subtracting zero is not addition for a floating-point -0. */
      if (trailing && ((token == TK_PLUS && value < CANDY_ADDI_BIAS) ||
          (token == TK_MINUS && value > 0 && value <= CANDY_ADDI_BIAS))) {
        rhs_is_immediate = true;
        rhs = (uint32_t)(CANDY_ADDI_BIAS + (token == TK_PLUS ? value : -value));
      }
      else if (trailing) {
        candy_wrap_t wrap;
        size_t k;
        candy_wrap_set_integer(&wrap, value);
        k = _add_const(self, &wrap);
        if (k < (1U << 9)) {
          rhs_is_const = true;
          rhs = (uint32_t)k;
        }
        else {
          rhs = (uint32_t)_newreg(self);
          _add_iabx(self, OP_LOADK, rhs, (uint32_t)k);
        }
      }
      else {
        /* A higher-precedence operator follows: the constant is only the head
         * of the right operand (e.g. `4 < 2 + 1`). */
        int c = _newreg(self);
        _load_integer(self, (uint32_t)c, value);
        rhs = (uint32_t)_expr_tail(self, c, -1, prec + 1);
      }
    }
    else {
      rhs = (uint32_t)_expr(self, -1, prec + 1);
    }

    if (pref >= 0 && _precedence(candy_lexer_lookahead(&self->ls)) < min_prec)
      result = pref;
    else if (lhs == (int)self->fs->freereg - 1 && !_is_local(self, lhs))
      result = lhs;
    else
      result = _newreg(self);

    op = rhs_is_immediate ? OP_ADDI : _binary_op(self, token, rhs_is_const);
    /* Reverse the comparison's operands after both expressions were evaluated. */
    if (!rhs_is_const && (token == TK_GREATER || token == TK_GEQUAL))
      _add_iabc(self, op, (uint32_t)result, rhs, (uint32_t)lhs);
    else
      _add_iabc(self, op, (uint32_t)result, (uint32_t)lhs, rhs);
    lhs = result;
  }
  return lhs;
}

/* A `[v1, v2, ...]` array literal: identical to `{v1, v2, ...}` but with the
 * JSON bracket spelling. */
static int _array_constructor(parser_t *self) {
  _next(self, '[');
  int array = _newreg(self);
  _add_iabx(self, OP_NEWCONTAINER, (uint32_t)array, 1);
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
      _load_integer(self, (uint32_t)count_reg, (candy_integer_t)count);
      int start_reg = _newreg(self);
      _load_integer(self, (uint32_t)start_reg, (candy_integer_t)start);
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

/* Parse one capture-list entry, a name or `&name`. */
static void _capture_entry(parser_t *self, funcstate_t *fs) {
  int8_t mode = _test(self, '&') ? 1 : 0;
  const candy_array_t *name = _next(self, TK_IDENT)->s;
  par_assert(fs->ncaptures < UINT8_MAX, "too many captures");
  candy_vector_append(&fs->captures, candy_gc_memory(self->ls.gc), self->ls.ctx,
    &(struct parser_capture){name, mode}, 1, sizeof(struct parser_capture));
  fs->ncaptures++;
}

/* Parse `name[&name, ...]` entries separated by commas. */
static void _capture_entries(parser_t *self, funcstate_t *fs) {
  do {
    _capture_entry(self, fs);
  } while (_test(self, ','));
}

/* A lambda expression: `def [captures](params) body end`. The capture list may
 * be omitted, which captures nothing. */
static int _def_expr(parser_t *self) {
  _next(self, TK_def);
  funcstate_t fs;
  _funcstate_open(&fs, self);
  fs.lambda = true;
  if (_test(self, '[')) {
    if (_test(self, '=')) {
      fs.capture_default = 0;
      if (!_test(self, ']')) {
        _next(self, ',');
        _capture_entries(self, &fs);
        _next(self, ']');
      }
    }
    else if (_test(self, '&')) {
      if (_test(self, ']')) {
        fs.capture_default = 1;
      }
      else if (candy_lexer_lookahead(&self->ls) == ',') {
        fs.capture_default = 1;
        _next(self, ',');
        _capture_entries(self, &fs);
        _next(self, ']');
      }
      else {
        /* `[&x, ...]`: explicit entries, the first one captured by reference. */
        const candy_array_t *first_capture = _next(self, TK_IDENT)->s;
        candy_vector_append(&fs.captures, candy_gc_memory(self->ls.gc), self->ls.ctx,
          &(struct parser_capture){first_capture, 1}, 1, sizeof(struct parser_capture));
        fs.ncaptures++;
        while (_test(self, ','))
          _capture_entry(self, &fs);
        _next(self, ']');
      }
    }
    else if (!_test(self, ']')) {
      _capture_entries(self, &fs);
      _next(self, ']');
    }
  }
  /* Every explicitly listed capture must name an enclosing variable. */
  for (uint8_t i = 0; i < fs.ncaptures; ++i)
    par_assert(_scope_has(self, fs.prev, ((struct parser_capture *)candy_vector_data(&fs.captures))[i].name), "cannot capture: %.*s",
      (int)candy_array_size(((struct parser_capture *)candy_vector_data(&fs.captures))[i].name), (char *)candy_array_data(((struct parser_capture *)candy_vector_data(&fs.captures))[i].name));
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
  if (!returns) {
    int null = fs.local_hwm > 1 ? (int)fs.local_hwm - 1 : _newreg(self);
    _add_iabx(self, OP_LOADK, (uint32_t)null, (uint32_t)_add_null(self));
    _add_iax(self, OP_RETURN, (uint32_t)null);
  }
  candy_proto_set_layout(fs.proto, (uint8_t)nparams, fs.nreg, fs.local_hwm);
  candy_proto_t *proto = fs.proto;
  _funcstate_close(&fs, self);
  candy_wrap_t wrap;
  candy_wrap_set_object(&wrap, (candy_object_t *)proto);
  int reg = _newreg(self);
  _add_iabx(self, OP_CLOSURE, (uint32_t)reg, (uint32_t)_add_const(self, &wrap));
  return reg;
}

static int _primary(parser_t *self) {
  candy_tokens_t token = candy_lexer_lookahead(&self->ls);
  int result;
  if (token == TK_MINUS) {
    /* unary minus `-x` lowers to `0 - x` */
    candy_lexer_next(&self->ls);
    int operand = _expr(self, -1, 30);
    int zero = _newreg(self);
    _load_integer(self, (uint32_t)zero, 0);
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
  if (token == TK_def) {
    return _def_expr(self);
  }
  if (token == TK_IDENT) {
    const candy_array_t *name = _next(self, TK_IDENT)->s;
    return _name_expr(self, name, 1);
  }
  par_assert(_is_literal(token), "unexpected token %s in expression", candy_token_str(token));
  result = _newreg(self);
  if (token == TK_INTEGER)
    _load_integer(self, (uint32_t)result, candy_lexer_next(&self->ls)->i);
  else
    _add_iabx(self, OP_LOADK, (uint32_t)result, (uint32_t)_add_meta(self, token));
  return result;
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

/* Parse a parenthesized condition and return the false jump to patch. */
static size_t _condition(parser_t *self) {
  candy_proto_t *proto = self->fs->proto;
  size_t start = _pc(self);
  size_t end;
  int reg;
  candy_inst_t inst;
  _next(self, '(');
  reg = _expr(self, -1, 0);
  _next(self, ')');
  end = _pc(self);
  if (end > start) {
    inst = ((const candy_inst_t *)candy_vector_data(candy_proto_get_inst(proto)))[end - 1];
    if (inst.iabc.a == (uint32_t)reg &&
        (inst.op == OP_LT || inst.op == OP_LTK || inst.op == OP_LE || inst.op == OP_LEK)) {
      /* Only the branch consumes the result. Keep the following JMP's full
       * target width instead of materializing a boolean in a register. */
      inst.iabc.a = inst.op == OP_LTK || inst.op == OP_LEK;
      inst.iabc.op = (inst.op == OP_LE || inst.op == OP_LEK) ? OP_TESTLE : OP_TESTLT;
      candy_proto_set_inst(proto, end - 1, inst);
      return _add_iax(self, OP_JMP, 0);
    }
  }
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
  if (after != '(' && after != '[' && after != TK_DOT && after != TK_SCOPE && _precedence(after) < 0) {
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

static int _name_value(parser_t *self, const candy_array_t *name) {
  int local = _local(self, name);
  if (local >= 0) return local;
  int reg = _newreg(self);
  _add_iabx(self, OP_LOADG, (uint32_t)reg, (uint32_t)_add_name_const(self, name));
  return reg;
}

static void _name_stmt(parser_t *self) {
  const candy_array_t *name = _next(self, TK_IDENT)->s;
  candy_tokens_t token = candy_lexer_lookahead(&self->ls);

  /* A statement postfix chain keeps its final table/key pair so assignment
   * writes the container instead of writing back to a temporary register. */
  if (token == TK_DOT || token == TK_SCOPE || token == '[') {
    int current = _name_value(self, name);
    for (;;) {
      bool method = token == TK_DOT;
      int key = _newreg(self);
      if (token == TK_DOT || token == TK_SCOPE) {
        candy_lexer_next(&self->ls);
        const candy_array_t *field = _next(self, TK_IDENT)->s;
        _add_iabx(self, OP_LOADK, (uint32_t)key, (uint32_t)_add_name_const(self, field));
      }
      else {
        candy_lexer_next(&self->ls);
        _expr(self, key, 0);
        _next(self, ']');
      }
      token = candy_lexer_lookahead(&self->ls);
      candy_opcodes_t op;
      if (token == '=' || _compound_op(self, token, &op)) {
        if (token == '=') {
          candy_lexer_next(&self->ls);
        }
        else {
          candy_lexer_next(&self->ls);
          int value = _expr(self, -1, 0);
          int old = _newreg(self);
          _add_iabc(self, OP_GETTABLE, (uint32_t)old, (uint32_t)current, (uint32_t)key);
          _add_iabc(self, op, (uint32_t)old, (uint32_t)old, (uint32_t)value);
          _add_iabc(self, OP_SETTABLE, (uint32_t)current, (uint32_t)key, (uint32_t)old);
          return;
        }
        int value = _expr(self, -1, 0);
        _add_iabc(self, OP_SETTABLE, (uint32_t)current, (uint32_t)key, (uint32_t)value);
        return;
      }
      if (token == '(') {
        candy_lexer_next(&self->ls);
        if (method)
          _call_method(self, current, key);
        else {
          int callee = _newreg(self);
          _add_iabc(self, OP_GETTABLE, (uint32_t)callee, (uint32_t)current, (uint32_t)key);
          _call_value(self, callee);
        }
        return;
      }
      int next = _newreg(self);
      _add_iabc(self, OP_GETTABLE, (uint32_t)next, (uint32_t)current, (uint32_t)key);
      current = next;
      token = candy_lexer_lookahead(&self->ls);
      if (token != TK_DOT && token != TK_SCOPE && token != '[')
        return;
    }
  }

  candy_opcodes_t op;
  if (_compound_op(self, token, &op)) {
    int current = _name_value(self, name);
    candy_lexer_next(&self->ls);
    int value = _expr(self, -1, 0);
    _add_iabc(self, op, (uint32_t)current, (uint32_t)current, (uint32_t)value);
    if (_local(self, name) < 0)
      _add_iabx(self, OP_SETG, (uint32_t)current, (uint32_t)_add_name_const(self, name));
    return;
  }
  if (token != '=') {
    _name_expr(self, name, 0);
    return;
  }
  candy_lexer_next(&self->ls);
  if (self->fs->prev != NULL) {
    /* Existing locals can receive the final RHS result directly. New locals
     * and captures still go through _bind_name after evaluating the RHS. */
    int local = _local(self, name);
    if (local > 0) {
      _expr(self, local, 0);
      return;
    }
  }
  else if (candy_lexer_lookahead(&self->ls) == TK_IDENT) {
    const candy_array_t *first = _next(self, TK_IDENT)->s;
    if (first == name && _test(self, '+')) {
      if (_fuse_global_add(self, name))
        return;
      int lhs = _name_expr(self, first, 1);
      int rhs = _expr(self, -1, 21);
      int sum = _newreg(self);
      _add_iabc(self, OP_ADD, (uint32_t)sum, (uint32_t)lhs, (uint32_t)rhs);
      _bind_name(self, name, _expr_tail(self, sum, -1, 0));
      return;
    }
    _bind_name(self, name, _expr_tail(self, _postfix(self, _name_expr(self, first, 1)), -1, 0));
    return;
  }
  _bind_name(self, name, _expr(self, -1, 0));
}

static void _class_stmt(parser_t *self) {
  _next(self, TK_class);
  const candy_array_t *name = _next(self, TK_IDENT)->s;
  int table = _newreg(self);
  _add_iabx(self, OP_NEWCONTAINER, (uint32_t)table, 0);
  _add_iabx(self, OP_SETG, (uint32_t)table, (uint32_t)_add_name_const(self, name));
  while (candy_lexer_lookahead(&self->ls) != TK_end) {
    _next(self, TK_def);
    const candy_array_t *method = _next(self, TK_IDENT)->s;
    /* Methods are compiled as ordinary named functions, then installed in the
     * class table. The name is temporary and does not affect explicit self. */
    funcstate_t fs;
    _funcstate_open(&fs, self);
    fs.name = method;
    _next(self, '(');
    if (!_test(self, ')')) {
      do { const candy_array_t *param = _next(self, TK_IDENT)->s; _declare_local(self, param); } while (_test(self, ','));
      _next(self, ')');
    }
    uint32_t nparams = fs.nlocal; fs.local_hwm = fs.freereg;
    bool returns = _block(self, true); _next(self, TK_end);
    if (!returns) { int nul = fs.local_hwm > 1 ? fs.local_hwm - 1 : _newreg(self); _add_iabx(self, OP_LOADK, nul, _add_null(self)); _add_iax(self, OP_RETURN, nul); }
    candy_proto_set_layout(fs.proto, (uint8_t)nparams, fs.nreg, fs.local_hwm);
    candy_proto_t *proto = fs.proto; _funcstate_close(&fs, self);
    candy_sclosure_t *closure = candy_sclosure_create(self->ls.gc, self->ls.ctx, proto, 0);
    candy_wrap_t wrap; candy_wrap_set_object(&wrap, (candy_object_t *)closure);
    int val = _newreg(self); _add_iabx(self, OP_LOADK, val, _add_const(self, &wrap));
    int key = _newreg(self); _add_iabx(self, OP_LOADK, key, _add_name_const(self, method));
    _add_iabc(self, OP_SETTABLE, table, key, val);
  }
  _next(self, TK_end);
}

static void _def_stmt(parser_t *self, bool exported) {
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
  candy_sclosure_t *closure = candy_sclosure_create(self->ls.gc, self->ls.ctx, proto, 0);
  candy_wrap_t wrap;
  candy_wrap_set_object(&wrap, (candy_object_t *)closure);
  int reg = _newreg(self);
  _add_iabx(self, OP_LOADK, (uint32_t)reg, (uint32_t)_add_const(self, &wrap));
  _bind_name(self, name, reg);
  if (exported && self->exports != NULL) {
    candy_wrap_t key;
    candy_wrap_set_object(&key, (const candy_object_t *)name);
    candy_err_t err = candy_table_set(self->exports, self->ls.gc, self->ls.ctx, &key, &wrap);
    par_assert(err == CANDY_OK, "could not export function");
  }
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

static candy_err_t _parse_chunk(parser_t *self, candy_object_t **entry, candy_object_t **out) {
  candy_gc_t *gc = self->ls.gc;
  candy_excep_t *ctx = self->ls.ctx;
  funcstate_t fs;
  _funcstate_open(&fs, self);
  candy_err_t err = candy_excep_try(ctx, _entry, self, out);
  _funcstate_close(&fs, self);
  if (err == CANDY_OK)
    *entry = (candy_object_t *)candy_sclosure_create(gc, ctx, fs.proto, 0);
  return err;
}

static void _compile_module(void *arg) {
  struct parse_module *module = (struct parse_module *)arg;
  parser_t *self = &module->parser;
  candy_gc_t *gc = self->ls.gc;
  candy_excep_t *ctx = self->ls.ctx;
  self->globals = candy_table_create(gc, ctx);
  self->exports = candy_table_create(gc, ctx);
  candy_object_t *out = NULL;
  candy_err_t err = _parse_chunk(self, &module->entry, &out);
  if (err != CANDY_OK)
    candy_excep_throw(ctx, err, out);
}

static void _module_set(parser_t *self, candy_table_t *table, const candy_wrap_t *key, const candy_wrap_t *value) {
  candy_err_t err = candy_table_set(table, self->ls.gc, self->ls.ctx, key, value);
  par_assert(err == CANDY_OK, "could not bind module");
}

static bool _path_sep(char ch) {
#if defined(_WIN32)
  return ch == '/' || ch == '\\';
#else
  return ch == '/';
#endif
}

/* Fold lexical aliases without querying the filesystem. Preserve leading
 * relative '..' components and never rewind past the path root. */
static void _normalize_path(char *path) {
  char *read = path;
#if defined(_WIN32)
  if (path[0] && path[1] == ':')
    read += 2;
#endif
  char *write = read;
  bool rooted = _path_sep(*read);
  if (rooted) {
    *write++ = '/';
    ++read;
#if defined(_WIN32)
    /* The server and share are the root of a UNC path. */
    if (read == path + 1 && _path_sep(*read)) {
      *write++ = '/';
      ++read;
      for (int i = 0; i < 2; ++i) {
        while (_path_sep(*read))
          ++read;
        while (*read && !_path_sep(*read))
          *write++ = *read++;
        if (_path_sep(*read)) {
          *write++ = '/';
          ++read;
        }
      }
    }
#endif
  }
  char *start = write;
  char *floor = start;
  while (*read) {
    while (_path_sep(*read))
      ++read;
    char *part = read;
    while (*read && !_path_sep(*read))
      ++read;
    size_t size = (size_t)(read - part);
    if (size == 0 || (size == 1 && part[0] == '.'))
      continue;
    bool parent = size == 2 && part[0] == '.' && part[1] == '.';
    if (parent && write > floor) {
      while (write > floor && write[-1] != '/')
        --write;
      if (write > floor)
        --write;
      continue;
    }
    if (parent && rooted)
      continue;
    if (write > start)
      *write++ = '/';
    memmove(write, part, size);
    write += size;
    if (parent)
      floor = write;
  }
  if (write == path)
    *write++ = '.';
  *write = '\0';
}

static const candy_array_t *_import_path(parser_t *self, const char *source, size_t prefix,
    const char *relative, size_t size) {
  candy_gc_t *gc = self->ls.gc;
  candy_excep_t *ctx = self->ls.ctx;
  candy_assert(ctx, gc, prefix < SIZE_MAX - size, CANDY_ERR_VM, "import path is too long");
  candy_array_t *buffer = candy_array_create(gc, ctx, CANDY_TYPE_CHAR);
  candy_array_resize(buffer, gc, ctx, prefix + size + 1);
  char *joined = (char *)candy_array_data(buffer);
  memcpy(joined, source, prefix);
  memcpy(joined + prefix, relative, size);
  joined[prefix + size] = '\0';
  _normalize_path(joined);
  return candy_array_create_const(gc, ctx, CANDY_TYPE_CHAR, joined, strlen(joined));
}

static void _load_import(parser_t *self, const candy_array_t *path, const candy_array_t *alias) {
  candy_gc_t *gc = self->ls.gc;
  candy_excep_t *ctx = self->ls.ctx;
  struct parse_graph *graph = self->graph;
  const char *relative = (const char *)candy_array_data(path);
  size_t size = candy_array_size(path);
  candy_assert(ctx, gc, size > 0 && memchr(relative, '\0', size) == NULL,
    CANDY_ERR_VM, "invalid import path");
  candy_assert(ctx, gc, relative[0] != '/'
#if defined(_WIN32)
    && relative[0] != '\\' && (size < 2 || relative[1] != ':')
#endif
    , CANDY_ERR_VM, "import requires a relative path");

  /* Normalize the entry lazily: chunks without imports need no path buffer. */
  if (!self->parent && self->path && !graph->compiled) {
    const candy_array_t *entry = _import_path(self, "", 0, self->path, strlen(self->path));
    self->path = (const char *)candy_array_data(entry);
  }
  const char *source = self->path ? self->path : "";
  size_t prefix = 0;
  for (size_t i = 0; source[i]; ++i)
    if (_path_sep(source[i]))
      prefix = i + 1;
  const candy_array_t *filename = _import_path(self, source, prefix, relative, size);
  const char *resolved = (const char *)candy_array_data(filename);

  for (const parser_t *it = self; it; it = it->parent)
    candy_assert(ctx, gc, !it->path || strcmp(it->path, resolved) != 0,
      CANDY_ERR_VM, "circular import '%s'", resolved);

  if (!graph->compiled) {
    graph->compiled = candy_table_create(gc, ctx);
    graph->pending = candy_array_create(gc, ctx, CANDY_TYPE_WRAP);
    graph->aliases = candy_array_create(gc, ctx, CANDY_TYPE_WRAP);
    graph->plan = candy_array_create(gc, ctx, CANDY_TYPE_WRAP);
    candy_wrap_t plan[2];
    candy_wrap_set_object(&plan[0], (candy_object_t *)graph->pending);
    candy_wrap_set_object(&plan[1], (candy_object_t *)graph->aliases);
    candy_array_append(graph->plan, gc, ctx, plan, 2);
    /* Only the root proto carries the initialization plan. */
    parser_t const *root = self;
    while (root->parent)
      root = root->parent;
    candy_proto_set_initializers(root->fs->proto, graph->plan);
  }
  candy_wrap_t key, exports;
  candy_wrap_set_object(&key, (const candy_object_t *)filename);
  exports = gc->modules ? *candy_table_get(gc->modules, gc, &key) : CANDY_WRAP_NULL;
  candy_assert(ctx, gc, candy_wrap_type(&exports) != CANDY_TYPE_BOOLEAN,
    CANDY_ERR_VM, "module initialization already in progress: '%s'", resolved);
  if (candy_wrap_type(&exports) == CANDY_TYPE_NULL)
    exports = *candy_table_get(graph->compiled, gc, &key);
  if (candy_wrap_type(&exports) == CANDY_TYPE_NULL) {
    FILE *file = fopen(resolved, "r");
    candy_assert(ctx, gc, file != NULL, CANDY_ERR_VM, "cannot open import '%s'", resolved);
    struct file_info reader = {file, resolved};
    struct parse_module module = {.parser = {.parent = self, .path = resolved, .graph = graph}};
    candy_lexer_init(&module.parser.ls, gc, ctx, file_reader, &reader);
    candy_object_t *out = NULL;
    candy_err_t err = candy_excep_try(ctx, _compile_module, &module, &out);
    bool read_failed = ferror(file);
    candy_lexer_deinit(&module.parser.ls);
    fclose(file);
    candy_assert(ctx, gc, !read_failed, CANDY_ERR_VM, "could not read module '%s'", resolved);
    if (err != CANDY_OK) {
      const char *message = out ? (const char *)candy_array_data((candy_array_t *)out) : "";
      int length = out ? (int)candy_array_size((candy_array_t *)out) : 0;
      candy_assert(ctx, gc, false, err, "in module '%s': %.*s", resolved, length, message);
    }
    candy_wrap_set_object(&exports, (candy_object_t *)module.parser.exports);
    _module_set(self, graph->compiled, &key, &exports);
    /* Postorder: dependencies are queued before their importer. */
    candy_wrap_t pending[] = {key, exports, CANDY_WRAP_NULL};
    candy_wrap_set_object(&pending[2], module.entry);
    candy_array_append(graph->pending, gc, ctx, pending, 3);
  }
  candy_wrap_set_object(&key, (const candy_object_t *)alias);
  if (self->globals)
    _module_set(self, self->globals, &key, &exports);
  else {
    candy_wrap_t binding[] = {key, exports};
    candy_array_append(graph->aliases, gc, ctx, binding, 2);
  }
}

static void _import_stmt(parser_t *self) {
  _next(self, TK_import);
  const candy_array_t *path = _next(self, TK_STRING)->s;
  _next(self, TK_as);
  const candy_array_t *name = _next(self, TK_IDENT)->s;
  _load_import(self, path, name);
  _test(self, ';');
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
  _load_integer(self, (uint32_t)idx, 0);
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
      case TK_class:   _class_stmt(self); break;
      case TK_def:      _def_stmt(self, false); break;
      case TK_import:
        par_assert(!stop_at_end, "import outside top level");
        _import_stmt(self);
        break;
      case TK_export:
        par_assert(!stop_at_end, "export outside top level");
        _next(self, TK_export);
        _def_stmt(self, true);
        _test(self, ';');
        break;
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

candy_err_t candy_parse(candy_gc_t *gc, candy_excep_t *ctx, candy_reader_t reader, void *arg, candy_object_t **out) {
  struct parse_graph graph = {0};
  const char *path = reader == file_reader ? ((const struct file_info *)arg)->path : NULL;
  parser_t parser = {.path = path, .graph = &graph};
  candy_lexer_init(&parser.ls, gc, ctx, reader, arg);
  candy_object_t *entry = NULL;
  candy_err_t err = _parse_chunk(&parser, &entry, out);
  candy_lexer_deinit(&parser.ls);
  if (err == CANDY_OK && out)
    *out = entry;
  return err;
}
