#pragma once

#include <stdio.h>
#include <string.h>

struct info_str {
  const char *exp;
  const int size;
  int offset;
};

struct info_file {
  FILE *f;
  const int size;
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
