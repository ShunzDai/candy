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
#include "src/candy_buffer.h"
#include "src/candy_parser.h"
#include "src/candy_reader.h"
#include "src/candy_vm.h"
#include <stdlib.h>
#include <string.h>

struct candy_state {
  /* global input-output buffer */
  candy_buffer_t *io;
  candy_vm_t *vm;
  void *ud;
};

static int _default_entry(candy_state_t *self) {
  return candy_pull_builtin(self)(self);
}

candy_state_t *candy_state_create(void *ud) {
  candy_state_t *self = (candy_state_t *)malloc(sizeof(struct candy_state));
  self->io = candy_buffer_create(CANDY_ATOMIC_IO_SIZE, sizeof(char), true);
  self->vm = candy_vm_create(self, _default_entry);
  self->ud = ud;
  return self;
}

int candy_state_delete(candy_state_t **self) {
  candy_vm_delete(&(*self)->vm);
  candy_buffer_delete(&(*self)->io);
  free(*self);
  *self = NULL;
  return 0;
}

int candy_dostring(candy_state_t *self, const char exp[]) {
  struct str_info info = {exp, strlen(exp), 0};
  if (candy_parse(self->io, string_reader, &info) == NULL) {
    printf("%s\n", (const char *)self->io->data);
    return -1;
  }
  return 0;
}

int candy_dofile(candy_state_t *self, const char name[]) {
  FILE *f = fopen(name, "r");
  if (f == NULL)
    return -1;
  fseek(f, 0, SEEK_END);
  size_t size = ftell(f);
  fseek(f, 0, SEEK_SET);
  struct file_info info = {f, size};
  if (candy_parse(self->io, file_reader, &info) == NULL) {
    printf("%s\n", (const char *)self->io->data);
    fclose(f);
    return -1;
  }
  fclose(f);
  return 0;
}

int candy_add_builtin(candy_state_t *self, candy_regist_t list[], size_t size) {
  return candy_vm_builtin(self->vm, list, size);
}

int candy_get_global(candy_state_t *self, const char name[]) {
  return candy_vm_get_global(self->vm, name);
}

int candy_call(candy_state_t *self, int nargs, int nresults) {
  return candy_vm_call(self->vm, nargs, nresults);
}

void *candy_ud(candy_state_t *self) {
  return self->ud;
}

candy_callinfo_t *candy_callinfo(candy_state_t *self) {
  return candy_vm_callinfo(self->vm);
}

void candy_push_integer(candy_state_t *self, const candy_integer_t val) {
  candy_vm_push_integer(self->vm, val);
}

void candy_push_float(candy_state_t *self, const candy_float_t val) {
  candy_vm_push_float(self->vm, val);
}

void candy_push_boolean(candy_state_t *self, const candy_boolean_t val) {
  candy_vm_push_boolean(self->vm, val);
}

void candy_push_string(candy_state_t *self, const char val[], size_t size) {
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

const char *candy_pull_string(candy_state_t *self, size_t *size) {
  return candy_vm_pull_string(self->vm, size);
}

candy_builtin_t candy_pull_builtin(candy_state_t *self) {
  return candy_vm_pull_builtin(self->vm);
}
