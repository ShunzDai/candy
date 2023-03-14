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
#include "src/candy_stack.h"
#include "src/candy_buffer.h"
#include "src/candy_wrap.h"
#include <stdlib.h>
#include <string.h>

struct candy_stack {
  candy_buffer_t *base;
  candy_stack_size_t size;
};

static inline size_t _get_buff_size(candy_stack_t *self) {
  return self->base->size;
}

static inline candy_wrap_t *_get_buff_data(candy_stack_t *self) {
  return (candy_wrap_t *)self->base->data;
}

candy_stack_t *candy_stack_create() {
  candy_stack_t *self = (candy_stack_t *)malloc(sizeof(candy_stack_t));
  self->base = candy_buffer_create(CANDY_ATOMIC_STACK_SIZE, sizeof(candy_wrap_t), false);
  self->size = 0;
  return self;
}

int candy_stack_delete(candy_stack_t **self) {
  while ((*self)->size)
    candy_wrap_deinit(candy_stack_pop(*self));
  candy_buffer_delete(&(*self)->base);
  free(*self);
  *self = NULL;
  return 0;
}

candy_stack_size_t candy_stack_size(candy_stack_t *self) {
  return self->size;
}

candy_wrap_t *candy_stack_view(candy_stack_t *self, candy_stack_size_t idx) {
  return (idx < self->size) ? &_get_buff_data(self)[idx] : NULL;
}

candy_wrap_t *candy_stack_base(candy_stack_t *self) {
  return &_get_buff_data(self)[0];
}

candy_wrap_t *candy_stack_top(candy_stack_t *self) {
  return &_get_buff_data(self)[self->size ? self->size - 1 : 0];
}

void candy_stack_push(candy_stack_t *self, candy_wrap_t *wrap) {
  if (self->size == _get_buff_size(self))
    candy_buffer_expand(self->base, CANDY_ATOMIC_STACK_SIZE, sizeof(candy_wrap_t));
  memset(&_get_buff_data(self)[self->size], 0, sizeof(candy_wrap_t));
  _get_buff_data(self)[self->size++] = *wrap;
}

candy_wrap_t *candy_stack_pop(candy_stack_t *self) {
  return self->size ? &_get_buff_data(self)[--self->size] : NULL;
}

candy_wraps_t candy_stack_type(candy_stack_t *self) {
  return candy_stack_top(self)->type;
}

void candy_stack_push_integer(candy_stack_t *self, const candy_integer_t val) {
  candy_wrap_t wrap;
  candy_wrap_init_integer(&wrap, &val, 1);
  candy_stack_push(self, &wrap);
}

void candy_stack_push_float(candy_stack_t *self, const candy_float_t val) {
  candy_wrap_t wrap;
  candy_wrap_init_float(&wrap, &val, 1);
  candy_stack_push(self, &wrap);
}

void candy_stack_push_boolean(candy_stack_t *self, const candy_boolean_t val) {
  candy_wrap_t wrap;
  candy_wrap_init_boolean(&wrap, &val, 1);
  candy_stack_push(self, &wrap);
}

void candy_stack_push_string(candy_stack_t *self, const char val[], size_t size) {
  candy_wrap_t wrap;
  candy_wrap_init_string(&wrap, val, size);
  candy_stack_push(self, &wrap);
}

candy_integer_t candy_stack_pull_integer(candy_stack_t *self) {
  return *candy_wrap_get_integer(candy_stack_pop(self));
}

candy_float_t candy_stack_pull_float(candy_stack_t *self) {
  return *candy_wrap_get_float(candy_stack_pop(self));
}

candy_boolean_t candy_stack_pull_boolean(candy_stack_t *self) {
  return *candy_wrap_get_boolean(candy_stack_pop(self));
}

const char *candy_stack_pull_string(candy_stack_t *self, size_t *size) {
  return candy_wrap_get_string(candy_stack_pop(self));
}
