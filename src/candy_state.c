#include "src/candy_state.h"
#include "src/candy_buffer.h"
#include "src/candy_parser.h"
#include "src/candy_vm.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

struct info_str {
  const char *exp;
  const int size;
  int offset;
};

struct info_file {
  FILE *f;
  const int size;
};

struct candy_state {
  candy_buffer_t buffer;
  candy_vm_t *vm;
  void *ud;
};

static inline int _string_reader(char *buff, const int max_len, void *ud) {
  struct info_str *info = (struct info_str *)ud;
  int len = (max_len > info->size - info->offset) ? (info->size - info->offset) : max_len;
  memcpy(buff, info->exp + info->offset, len);
  info->offset += len;
  return len;
}

static inline int _file_reader(char *buff, const int max_len, void *ud) {
  struct info_file *info = (struct info_file *)ud;
  int len = (max_len > info->size - (int)ftell(info->f)) ? (info->size - (int)ftell(info->f)) : max_len;
  fread(buff, sizeof(char), len, info->f);
  if ((int)ftell(info->f) == info->size)
    buff[len] = '\0';
  return len;
}

candy_state_t *candy_state_create(void *ud) {
  candy_state_t *self = (candy_state_t *)malloc(sizeof(struct candy_state));
  candy_buffer_init(&self->buffer);
  self->vm = candy_vm_create(self);
  self->ud = ud;
  return self;
}

int candy_state_delete(candy_state_t **self) {
  candy_vm_delete(&(*self)->vm);
  candy_buffer_deinit(&(*self)->buffer);
  free(*self);
  *self = NULL;
  return 0;
}

int candy_dostring(candy_state_t *self, const char exp[]) {
  struct info_str info = {exp, (int)strlen(exp) + 1, 0};
  if (candy_parse(&self->buffer, _string_reader, &info) == NULL) {
    printf("%s\n", candy_buffer_get_data(&self->buffer));
    return -1;
  }
  return 0;
}

int candy_dofile(candy_state_t *self, const char name[]) {
  FILE *f = fopen(name, "r");
  if (f == NULL)
    return -1;
  fseek(f, 0, SEEK_END);
  int size = (int)ftell(f);
  fseek(f, 0, SEEK_SET);
  struct info_file info = {f, size};
  if (candy_parse(&self->buffer, _file_reader, &info) == NULL) {
    printf("%s\n", candy_buffer_get_data(&self->buffer));
    return -1;
  }
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

candy_callinfo_t *candy_callinfo(candy_state_t *self) {
  return NULL;
}

void candy_push_integer(candy_state_t *self, candy_integer_t val) {
  candy_vm_push_integer(self->vm, &val, 1);
}

void candy_push_float(candy_state_t *self, candy_float_t val) {
  candy_vm_push_float(self->vm, &val, 1);
}

void candy_push_boolean(candy_state_t *self, candy_boolean_t val) {
  candy_vm_push_boolean(self->vm, &val, 1);
}

void candy_push_string(candy_state_t *self, char *val, int size) {
  candy_vm_push_string(self->vm, val, size);
}

candy_integer_t candy_pull_integer(candy_state_t *self) {
  return *candy_vm_pull_integer(self->vm, NULL);
}

candy_float_t candy_pull_float(candy_state_t *self) {
  return *candy_vm_pull_float(self->vm, NULL);
}

candy_boolean_t candy_pull_boolean(candy_state_t *self) {
  return *candy_vm_pull_boolean(self->vm, NULL);
}

char *candy_pull_string(candy_state_t *self, int *size) {
  return candy_vm_pull_string(self->vm, size);
}
