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
#ifndef CANDY_CORE_READER_H
#define CANDY_CORE_READER_H
#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#include <stdio.h>

struct str_info {
  const char *exp;
  const size_t size;
  size_t offset;
};

struct file_info {
  FILE *f;
};

int string_reader(char buffer[], const size_t max_len, void *arg);

int file_reader(char buffer[], const size_t max_len, void *arg);

#ifdef __cplusplus
}
#endif /* __cplusplus */
#endif /* CANDY_CORE_READER_H */
