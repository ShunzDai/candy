#include "src/candy_stack.h"
#include "src/candy_wrap.h"
#include <stdlib.h>
#include <string.h>

struct candy_stack {
  candy_wrap_t *base;
  candy_stack_size_t max_size;
  candy_stack_size_t size;
};

candy_stack_t *candy_stack_create(candy_stack_size_t size) {
  candy_stack_t *self = (candy_stack_t *)malloc(sizeof(candy_stack_t));
  self->base = (candy_wrap_t *)calloc(size, sizeof(candy_wrap_t));
  self->max_size = size;
  self->size = 0;
  return self;
}

int candy_stack_delete(candy_stack_t **self) {
  while ((*self)->size)
    candy_wrap_deinit(candy_stack_pop(*self));
  free((*self)->base);
  *self = NULL;
  return 0;
}

candy_stack_size_t candy_stack_size(candy_stack_t *self) {
  return self->size;
}

candy_wrap_t *candy_stack_view(candy_stack_t *self, candy_stack_size_t idx) {
  return (idx < self->size) ? &self->base[idx] : NULL;
}

candy_wrap_t *candy_stack_base(candy_stack_t *self) {
  return &self->base[0];
}

candy_wrap_t *candy_stack_top(candy_stack_t *self) {
  return &self->base[self->size ? self->size - 1 : 0];
}

void candy_stack_push(candy_stack_t *self, candy_wrap_t *wrap) {
  if (self->size == self->max_size) {
    void *new = calloc(self->max_size * 2, sizeof(candy_wrap_t));
    memcpy(new, self->base, sizeof(candy_wrap_t) * self->max_size);
    self->max_size *= 2;
    free(self->base);
    self->base = new;
  }
  memset(&self->base[self->size], 0, sizeof(candy_wrap_t));
  self->base[self->size++] = *wrap;
}

candy_wrap_t *candy_stack_pop(candy_stack_t *self) {
  return self->size ? &self->base[--self->size] : NULL;
}

void candy_stack_push_integer(candy_stack_t *self, candy_integer_t *val, int size) {
  candy_wrap_t wrap;
  candy_wrap_init_integer(&wrap, val, size);
  candy_stack_push(self, &wrap);
}

void candy_stack_push_float(candy_stack_t *self, candy_float_t *val, int size) {
  candy_wrap_t wrap;
  candy_wrap_init_float(&wrap, val, size);
  candy_stack_push(self, &wrap);
}

void candy_stack_push_boolean(candy_stack_t *self, candy_boolean_t *val, int size) {
  candy_wrap_t wrap;
  candy_wrap_init_boolean(&wrap, val, size);
  candy_stack_push(self, &wrap);
}

void candy_stack_push_string(candy_stack_t *self, char *val, int size) {
  candy_wrap_t wrap;
  candy_wrap_init_string(&wrap, val, size);
  candy_stack_push(self, &wrap);
}

candy_integer_t *candy_stack_pull_integer(candy_stack_t *self, int *size) {
  return candy_wrap_get_integer(candy_stack_pop(self), size);
}

candy_float_t *candy_stack_pull_float(candy_stack_t *self, int *size) {
  return candy_wrap_get_float(candy_stack_pop(self), size);
}

candy_boolean_t *candy_stack_pull_boolean(candy_stack_t *self, int *size) {
  return candy_wrap_get_boolean(candy_stack_pop(self), size);
}

char *candy_stack_pull_string(candy_stack_t *self, int *size) {
  return candy_wrap_get_string(candy_stack_pop(self), size);
}
