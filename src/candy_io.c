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
#include "src/candy_io.h"
#include "src/candy_lib.h"
#include <stdio.h>
#include <stdarg.h>
#include <string.h>

static int _vsnprint(candy_io_t *self, const char format[], va_list ap) {
  va_list head;
  memcpy(&head, &ap, sizeof(va_list));
  int len = vsnprintf(NULL, 0, format, ap) + 1;
  if (candy_wrap_size(&self->buff) < len)
    candy_wrap_append(&self->buff, NULL, candy_wrap_size(&self->buff));
  return vsnprintf((char *)candy_wrap_get_string(&self->buff), len, format, head);
}

int candy_io_try_catch(candy_io_t *self, candy_try_catch_cb_t cb, void *handle, void *ud) {
  if (setjmp(self->env))
    goto catch;
  cb(handle, ud);
  return 0;
  catch:
  return -1;
}

void candy_io_throw(candy_io_t *self, const char format[], ...) {
  va_list ap;
  va_start(ap, format);
  _vsnprint(self, format, ap);
  va_end(ap);
  longjmp(self->env, 1);
}

int candy_io_init(candy_io_t *self) {
  candy_wrap_set_string(&self->buff, NULL, CANDY_DEFAULT_IO_SIZE);
  return 0;
}

int candy_io_deinit(candy_io_t *self) {
  candy_wrap_deinit(&self->buff);
  return 0;
}
