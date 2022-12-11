/**
  * Copyright 2022 ShunzDai
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
