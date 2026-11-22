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
#include "core/candy_upvalue.h"
#include "core/candy_object.h"
#include "core/candy_gc.h"
#include "core/candy_wrap.h"
#include "core/candy_vector.h"

struct candy_upvalue {
  candy_object_t header;
  candy_object_t *gray;
  /** @brief stable stack descriptor and index, NULL stack once closed */
  candy_vector_t *stack;
  ptrdiff_t slot;
  /** @brief value kept after the slot goes away */
  candy_wrap_t value;
  /** @brief next open upvalue of the same frame, sorted by slot */
  candy_upvalue_t *next;
};

static candy_err_t _upvalue_delete(candy_upvalue_t *self, candy_gc_t *gc, void *arg) {
  candy_gc_free(gc, self, sizeof(candy_upvalue_t));
  return CANDY_OK;
}

static candy_err_t _upvalue_color(candy_upvalue_t *self, candy_gc_t *gc, void *arg) {
  self->gray = candy_gc_gray_swap(gc, (candy_object_t *)self);
  candy_object_set_mark((candy_object_t *)self, MARK_GRAY);
  return CANDY_OK;
}

static candy_err_t _upvalue_diffuse(candy_upvalue_t *self, candy_gc_t *gc, void *arg) {
  candy_gc_gray_swap(gc, self->gray);
  candy_object_set_mark((candy_object_t *)self, MARK_DARK);
  return candy_wrap_mark(candy_upvalue_view(self), gc);
}

static candy_upvalue_t *_upvalue_new(candy_gc_t *gc, candy_excep_t *ctx, candy_vector_t *stack, ptrdiff_t slot) {
  candy_upvalue_t *self = (candy_upvalue_t *)candy_gc_add(gc, ctx, CANDY_TYPE_UPVAL, sizeof(candy_upvalue_t));
  self->gray = NULL;
  self->stack = stack;
  self->slot = slot;
  self->next = NULL;
  candy_wrap_set_none(&self->value);
  return self;
}

candy_upvalue_t *candy_upvalue_create_copy(candy_gc_t *gc, candy_excep_t *ctx, const candy_wrap_t *value) {
  candy_upvalue_t *self = _upvalue_new(gc, ctx, NULL, 0);
  self->value = *value;
  return self;
}

candy_err_t candy_upvalue_handler(candy_upvalue_t *self, candy_gc_t *gc, candy_events_t evt, void *arg) {
  switch (evt) {
    case EVT_DELETE:  return _upvalue_delete(self, gc, arg);
    case EVT_COLOR:   return _upvalue_color(self, gc, arg);
    case EVT_DIFFUSE: return _upvalue_diffuse(self, gc, arg);
    default:          return CANDY_ERR;
  }
}

candy_wrap_t *candy_upvalue_view(candy_upvalue_t *self) {
  return self->stack ? (candy_wrap_t *)candy_vector_data(self->stack) + self->slot : &self->value;
}

static void _upvalue_close(candy_upvalue_t *self) {
  if (self->stack != NULL) {
    self->value = *candy_upvalue_view(self);
    self->stack = NULL;
    candy_object_set_mask((candy_object_t *)self, MASK_OBJECT);
  }
}

candy_upvalue_t *candy_upvalue_open(candy_upvalue_t **list, candy_gc_t *gc, candy_excep_t *ctx, candy_vector_t *stack, ptrdiff_t slot) {
  candy_upvalue_t *prev = NULL;
  candy_upvalue_t *next = *list;
  while (next != NULL && next->slot > slot) {
    prev = next;
    next = next->next;
  }
  if (next != NULL && next->slot == slot)
    return next;
  candy_upvalue_t *self = _upvalue_new(gc, ctx, stack, slot);
  /* The open list owns a GC root until the slot is detached. */
  candy_object_set_mask((candy_object_t *)self, MASK_OBJECT | MASK_ROOT);
  self->next = next;
  if (prev != NULL)
    prev->next = self;
  else
    *list = self;
  return self;
}

void candy_upvalue_close_all(candy_upvalue_t **list, ptrdiff_t from) {
  candy_upvalue_t *it = *list;
  while (it != NULL && it->slot >= from) {
    _upvalue_close(it);
    it = it->next;
  }
  *list = it;
}
