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
#include "src/candy_parser.h"
#include "src/candy_reader.h"
#include "src/candy_block.h"
#include "src/candy_vm.h"
#include <stdlib.h>
#include <string.h>

struct candy_state {
  candy_vm_t vm;
  void *ud;
};

candy_state_t *candy_state_create(int argc, const char *argv[], void *ud) {
  candy_state_t *self = (candy_state_t *)calloc(1, sizeof(struct candy_state));
  candy_vm_init(&self->vm);
  self->ud = ud;
  return self;
}

int candy_state_delete(candy_state_t **self) {
  candy_vm_deinit(&(*self)->vm);
  free(*self);
  *self = NULL;
  return 0;
}

int candy_dostring(candy_state_t *self, const char exp[]) {
  struct str_info info = {exp, strlen(exp), 0};
  candy_block_t *block = candy_parse(&self->vm.io, string_reader, &info);
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
  candy_block_t *block = candy_parse(&self->vm.io, file_reader, &info);
  fclose(f);
  if (block == NULL)
    return -1;
  return 0;
}

const char *candy_error(candy_state_t *self) {
  return candy_wrap_get_string(&self->vm.io.buff);
}

int candy_add_builtin(candy_state_t *self, candy_regist_t list[], size_t size) {
  return candy_vm_builtin(&self->vm, list, size);
}

int candy_fprint(candy_state_t *self, const candy_wrap_t *wrap, FILE *out) {
  return candy_wrap_fprint(wrap, out, -1);
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

#define CANDY_TYPE_STATE_IMPL
#include "src/candy_type.list"
