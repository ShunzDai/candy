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
#include "core/candy_proto.h"
#include "core/candy_object.h"
#include "core/candy_vector.h"
#include "core/candy_wrap.h"
#include "core/candy_gc.h"
#include "core/candy_table.h"
#include <string.h>

#define CANDY_CONST_LINEAR_MAX 8

struct candy_proto {
  candy_object_t header;
  candy_object_t *gray;
  candy_vector_t cst;
  candy_vector_t inst;
  candy_table_t *map;
  uint8_t nparams;
  uint8_t nreg;
  uint8_t nlocal;
};

/* Object constants such as closures, protos and tables are identities, not
 * values: two of them can never be equal, so hashing them is wasted work. */
static bool _internable(const candy_wrap_t *wrap) {
  uint8_t mask = candy_wrap_mask(wrap);
  return !(mask & MASK_OBJECT) || (mask & MASK_ARRAY);
}

static candy_err_t _proto_delete(candy_proto_t *self, candy_gc_t *gc, void *arg) {
  candy_vector_deinit(&self->inst, candy_gc_memory(gc), sizeof(candy_inst_t));
  candy_vector_deinit(&self->cst, candy_gc_memory(gc), sizeof(candy_wrap_t));
  candy_gc_free(gc, self, sizeof(candy_proto_t));
  return CANDY_OK;
}

static candy_err_t _proto_color(candy_proto_t *self, candy_gc_t *gc, void *arg) {
  self->gray = candy_gc_gray_swap(gc, (candy_object_t *)self);
  candy_object_set_mark((candy_object_t *)self, MARK_GRAY);
  return CANDY_OK;
}

static candy_err_t _proto_diffuse(candy_proto_t *self, candy_gc_t *gc, void *arg) {
  candy_gc_gray_swap(gc, self->gray);
  candy_object_set_mark((candy_object_t *)self, MARK_DARK);
  if (self->map) {
    candy_err_t err = candy_gc_mark(gc, (candy_object_t *)self->map);
    if (err != CANDY_OK)
      return err;
  }
  const candy_wrap_t *constants = (const candy_wrap_t *)candy_vector_data(&self->cst);
  for (size_t index = 0; index < candy_vector_size(&self->cst); ++index) {
    candy_err_t err = candy_wrap_mark(constants + index, gc);
    if (err != CANDY_OK)
      return err;
  }
  return CANDY_OK;
}

candy_proto_t *candy_proto_create(candy_gc_t *gc, candy_excep_t *ctx) {
  candy_proto_t *self = (candy_proto_t *)candy_gc_add(gc, ctx, CANDY_TYPE_PROTO, sizeof(candy_proto_t));
  candy_vector_init(&self->cst);
  candy_vector_init(&self->inst);
  self->map = NULL;
  self->nparams = 0;
  self->nreg = 0;
  self->nlocal = 0;
  return self;
}

candy_err_t candy_proto_handler(candy_proto_t *self, candy_gc_t *gc, candy_events_t evt, void *arg) {
  switch (evt) {
    case EVT_DELETE:  return _proto_delete(self, gc, arg);
    case EVT_COLOR:   return _proto_color(self, gc, arg);
    case EVT_DIFFUSE: return _proto_diffuse(self, gc, arg);
    default:          return CANDY_ERR;
  }
}

size_t candy_proto_add_const_unique(candy_proto_t *self, candy_gc_t *gc, candy_excep_t *ctx, const candy_wrap_t *wrap) {
  size_t pos = candy_vector_size(&self->cst);
  candy_vector_append(&self->cst, candy_gc_memory(gc), ctx, wrap, 1, sizeof(candy_wrap_t));
  return pos;
}

size_t candy_proto_add_const(candy_proto_t *self, candy_gc_t *gc, candy_excep_t *ctx, const candy_wrap_t *wrap) {
  size_t pos = candy_vector_size(&self->cst);
  const candy_wrap_t *constants = (const candy_wrap_t *)candy_vector_data(&self->cst);
  /* Small protos are cheaper to scan than to give a hash table. */
  if (pos <= CANDY_CONST_LINEAR_MAX) {
    for (size_t index = 0; index < pos; ++index)
      if (candy_wrap_equal(constants + index, wrap))
        return index;
  }
  else if (_internable(wrap)) {
    if (self->map == NULL)
      self->map = candy_table_create(gc, ctx);
    const candy_wrap_t *val = candy_table_get(self->map, gc, wrap);
    if (candy_wrap_type(val) == CANDY_TYPE_INTEGER)
      return (size_t)candy_wrap_get_integer(val);
    candy_proto_add_const_unique(self, gc, ctx, wrap);
    candy_wrap_t index;
    candy_wrap_set_integer(&index, (candy_integer_t)pos);
    candy_table_set(self->map, gc, ctx, wrap, &index);
    return pos;
  }
  return candy_proto_add_const_unique(self, gc, ctx, wrap);
}

size_t candy_proto_add_inst(candy_proto_t *self, candy_gc_t *gc, candy_excep_t *ctx, candy_inst_t inst) {
  size_t pos = candy_vector_size(&self->inst);
  candy_vector_append(&self->inst, candy_gc_memory(gc), ctx, &inst, 1, sizeof(candy_inst_t));
  return pos;
}

void candy_proto_set_inst(candy_proto_t *self, size_t pos, candy_inst_t inst) {
  ((candy_inst_t *)candy_vector_data(&self->inst))[pos] = inst;
}

const candy_vector_t *candy_proto_get_const(const candy_proto_t *self) {
  return &self->cst;
}

const candy_vector_t *candy_proto_get_inst(const candy_proto_t *self) {
  return &self->inst;
}

void candy_proto_set_layout(candy_proto_t *self, uint8_t nparams, uint8_t nreg, uint8_t nlocal) {
  self->nparams = nparams;
  self->nreg = nreg;
  self->nlocal = nlocal;
  /* The constant cache table only serves compile-time dedup; once the layout
   * is fixed the proto no longer appends constants, so drop it for the GC. */
  self->map = NULL;
}

uint8_t candy_proto_get_nparams(const candy_proto_t *self) {
  return self->nparams;
}

uint8_t candy_proto_get_nreg(const candy_proto_t *self) {
  return self->nreg;
}

uint8_t candy_proto_get_nlocal(const candy_proto_t *self) {
  return self->nlocal;
}
