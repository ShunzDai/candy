#include "src/candy_state.h"
#include "src/candy_vm.h"
#include <stdlib.h>
#include <stdio.h>

struct candy_state {
  candy_vm_t *vm;
  void *ud;
};

candy_state_t *candy_state_create(void *ud) {
  candy_state_t *self = (candy_state_t *)malloc(sizeof(struct candy_state));
  self->vm = candy_vm_create(self);
  self->ud = ud;
  return self;
}

int candy_state_delete(candy_state_t **self) {
  candy_vm_delete(&(*self)->vm);
  free(*self);
  *self = NULL;
  return 0;
}

void *candy_ud(candy_state_t *self) {
  return self->ud;
}

void candy_regist_entry(candy_state_t *self, candy_cfunc_t entry) {
  candy_vm_regist_entry(self->vm, entry);
}

void candy_regist_cfuncs(candy_state_t *self, const char obj[], candy_regist_t list[], unsigned size) {
  for (unsigned idx = 0; idx < size; idx++) {
    printf("regist func '%s' in >>>\n", list[idx].name);
    // self->entry ? self->entry(sta, list[idx].cfunc) : list[idx].cfunc(sta, self->ud);
    printf("regist func '%s' out <<<\n", list[idx].name);
  }
}

void candy_push_integer(candy_state_t *self, candy_integer_t val) {
  candy_vm_push_integer(self->vm, val);
}

void candy_push_float(candy_state_t *self, candy_float_t val) {
  candy_vm_push_float(self->vm, val);
}

void candy_push_boolean(candy_state_t *self, candy_boolean_t val) {
  candy_vm_push_boolean(self->vm, val);
}

void candy_push_string(candy_state_t *self, char *val, int size) {
  candy_vm_push_string(self->vm, val, size);
}

candy_integer_t candy_pull_integer(candy_state_t *self) {
  return candy_vm_pull_integer(self->vm);
}

candy_float_t candy_pull_float(candy_state_t *self) {
  return candy_vm_pull_float(self->vm);
}

candy_boolean_t candy_pull_boolean(candy_state_t *self) {
  return candy_vm_pull_boolean(self->vm);
}

char *candy_pull_string(candy_state_t *self, int *size) {
  return candy_vm_pull_string(self->vm, size);
}
