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
#include "src/candy_buffer.h"
#include <stdio.h>
#include <stdarg.h>

void candy_assert(candy_buffer_t *self, const char format[], ...) {
  va_list ap;
  va_start(ap, format);
  vsnprintf(self->data, self->size, format, ap);
  va_end(ap);
  longjmp(self->rollback, 1);
}
