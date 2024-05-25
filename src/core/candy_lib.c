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
#include "core/candy_lib.h"
#include <stdlib.h>
#include <string.h>

long strntol(const char nptr[], size_t size, char *endptr[], int base) {
  char buf[size + 1];
  memcpy(buf, nptr, size);
  buf[size] = '\0';
  long res = strtol(buf, endptr, base);
  if (endptr)
    *endptr = (char *)nptr + (*endptr - buf);
  return res;
}

double strntod(const char nptr[], size_t size, char *endptr[]) {
  char buf[size + 1];
  memcpy(buf, nptr, size);
  buf[size] = '\0';
  double res = strtod(buf, endptr);
  if (endptr)
    *endptr = (char *)nptr + (*endptr - buf);
  return res;
}
