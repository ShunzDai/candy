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
#include "core/candy_userdef.h"
#include "core/candy_object.h"
#include "core/candy_gc.h"
#include <string.h>

struct candy_userdef {
  candy_object_t header;
  candy_object_t *gray;
  size_t size;
  uint8_t data[];
};

candy_userdef_t *candy_userdef_create(candy_gc_t *gc, candy_exce_t *ctx, const void *data, size_t size) {
  candy_userdef_t *self = (candy_userdef_t *)candy_gc_add(gc, ctx, CANDY_TYPE_UDHVY, sizeof(struct candy_userdef) + size);
  self->gray = NULL;
  self->size = size;
  memcpy(self->data, data, size);
  return self;
}

int candy_userdef_delete(candy_userdef_t *self, candy_gc_t *gc) {
  candy_gc_free(gc, self, sizeof(struct candy_userdef) + self->size);
  return 0;
}
