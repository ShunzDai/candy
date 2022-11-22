#pragma once

#include <stdio.h>
#include <string.h>

struct reader_s {
  const char *code;
  const int size;
  int offset;
};

struct reader_f {
  FILE *f;
  const int size;
};

static inline int reader_str(char *buff, const int max_len, void *ud) {
  reader_s *info = (reader_s *)ud;
  int len = (max_len > info->size - info->offset) ? (info->size - info->offset) : max_len;
  memcpy(buff, info->code + info->offset, len);
  info->offset += len;
  return len;
}

static inline int reader_file(char *buff, const int max_len, void *ud) {
  reader_f *info = (reader_f *)ud;
  int len = (max_len > info->size - (int)ftell(info->f)) ? (info->size - (int)ftell(info->f)) : max_len;
  fread(buff, sizeof(char), len, info->f);
  if ((int)ftell(info->f) == info->size)
    buff[len] = '\0';
  return len;
}
