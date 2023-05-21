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
#include "src/candy_state.h"
#include "src/candy_io.h"
#include "src/candy_parser.h"
#include "src/candy_reader.h"
#include "src/candy_block.h"
#include "src/candy_vm.h"
#include <stdlib.h>
#include <string.h>

struct candy_state {
  /* global input-output buffer */
  candy_io_t io;
  candy_vm_t vm;
  void *ud;
};

candy_state_t *candy_state_create(int argc, const char *argv[], void *ud) {
  candy_state_t *self = (candy_state_t *)calloc(1, sizeof(struct candy_state));
  candy_io_init(&self->io);
  candy_vm_init(&self->vm, self);
  self->ud = ud;
  return self;
}

int candy_state_delete(candy_state_t **self) {
  candy_vm_deinit(&(*self)->vm);
  candy_io_deinit(&(*self)->io);
  free(*self);
  *self = NULL;
  return 0;
}

int candy_dostring(candy_state_t *self, const char exp[]) {
  struct str_info info = {exp, strlen(exp), 0};
  candy_block_t *block = candy_parse(&self->io, string_reader, &info);
  if (block == NULL)
    return -1;
  candy_vm_execute(&self->vm, block);
  candy_block_delete(&block);
  return 0;
}

int candy_dofile(candy_state_t *self, const char name[]) {
  FILE *f = fopen(name, "r");
  if (f == NULL)
    return perror(NULL), -1;
  fseek(f, 0, SEEK_END);
  size_t size = ftell(f);
  fseek(f, 0, SEEK_SET);
  struct file_info info = {f, size};
  candy_block_t *block = candy_parse(&self->io, file_reader, &info);
  fclose(f);
  if (block == NULL)
    return -1;
  return 0;
}

int candy_add_builtin(candy_state_t *self, candy_regist_t list[], size_t size) {
  return candy_vm_builtin(&self->vm, list, size);
}

int candy_fprint_top(candy_state_t *self, FILE *out) {
  return candy_wrap_fprint(candy_vm_pop(&self->vm), out, -1);
}

int candy_fprint_global(candy_state_t *self, FILE *out) {
  return candy_vm_fprint_global(&self->vm, out);
}

int candy_set_global(candy_state_t *self, const char name[]) {
  return candy_vm_set_global(&self->vm, name);
}

int candy_get_global(candy_state_t *self, const char name[]) {
  return candy_vm_get_global(&self->vm, name);
}

int candy_call(candy_state_t *self, int nargs, int nresults) {
  return candy_vm_call(&self->vm, nargs, nresults);
}

void *candy_ud(candy_state_t *self) {
  return self->ud;
}

void candy_push_integer(candy_state_t *self, const candy_integer_t val[], size_t size) {
  candy_wrap_t wrap;
  candy_wrap_set_integer(&wrap, val, size);
  candy_vm_push(&self->vm, &wrap);
}

void candy_push_float(candy_state_t *self, const candy_float_t val[], size_t size) {
  candy_wrap_t wrap;
  candy_wrap_set_float(&wrap, val, size);
  candy_vm_push(&self->vm, &wrap);
}

void candy_push_boolean(candy_state_t *self, const candy_boolean_t val[], size_t size) {
  candy_wrap_t wrap;
  candy_wrap_set_boolean(&wrap, val, size);
  candy_vm_push(&self->vm, &wrap);
}

void candy_push_string(candy_state_t *self, const char val[], size_t size) {
  candy_wrap_t wrap;
  candy_wrap_set_string(&wrap, val, size);
  candy_vm_push(&self->vm, &wrap);
}

void candy_push_ud(candy_state_t *self, const void *val[], size_t size) {
  candy_wrap_t wrap;
  candy_wrap_set_ud(&wrap, val, size);
  candy_vm_push(&self->vm, &wrap);
}

void candy_push_builtin(candy_state_t *self, const candy_builtin_t val[], size_t size) {
  candy_wrap_t wrap;
  candy_wrap_set_builtin(&wrap, val, size);
  candy_vm_push(&self->vm, &wrap);
}

const candy_integer_t *candy_pull_integer(candy_state_t *self, size_t *size) {
  const candy_wrap_t *wrap = candy_vm_pop(&self->vm);
  if (size)
    *size = wrap->size;
  return candy_wrap_get_integer(wrap);
}

const candy_float_t *candy_pull_float(candy_state_t *self, size_t *size) {
  const candy_wrap_t *wrap = candy_vm_pop(&self->vm);
  if (size)
    *size = wrap->size;
  return candy_wrap_get_float(wrap);
}

const candy_boolean_t *candy_pull_boolean(candy_state_t *self, size_t *size) {
  const candy_wrap_t *wrap = candy_vm_pop(&self->vm);
  if (size)
    *size = wrap->size;
  return candy_wrap_get_boolean(wrap);
}

const char *candy_pull_string(candy_state_t *self, size_t *size) {
  const candy_wrap_t *wrap = candy_vm_pop(&self->vm);
  if (size)
    *size = wrap->size;
  return candy_wrap_get_string(wrap);
}

const void **candy_pull_ud(candy_state_t *self, size_t *size) {
  const candy_wrap_t *wrap = candy_vm_pop(&self->vm);
  if (size)
    *size = wrap->size;
  return candy_wrap_get_ud(wrap);
}

const candy_builtin_t *candy_pull_builtin(candy_state_t *self, size_t *size) {
  const candy_wrap_t *wrap = candy_vm_pop(&self->vm);
  if (size)
    *size = wrap->size;
  return candy_wrap_get_builtin(wrap);
}
