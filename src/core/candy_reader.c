/**
  * Copyright 2022-2024 ShunzDai
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
#include "core/candy_reader.h"
#include <string.h>

int string_reader(char buffer[], const size_t max_len, void *arg) {
  struct str_info *info = (struct str_info *)arg;
  size_t residual = info->size - info->offset;
  size_t len = (max_len > residual) ? residual : max_len;
  memcpy(buffer, info->exp + info->offset, len);
  info->offset += len;
  if (info->offset == info->size && len < max_len)
    buffer[len++] = '\0';
  return len;
}

int file_reader(char buffer[], const size_t max_len, void *arg) {
  struct file_info *info = (struct file_info *)arg;
  size_t len = fread(buffer, sizeof(char), max_len, info->f);
  if (len < max_len)
    buffer[len++] = '\0';
  return len;
}
