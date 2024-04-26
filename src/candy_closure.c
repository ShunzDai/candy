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
#include "src/candy_closure.h"
#include "src/candy_object.h"
#include "src/candy_gc.h"

struct candy_cclosure {
  candy_object_t header;
  candy_cfunc_t cfunc;
};

struct candy_sclosure {
  candy_object_t header;
  candy_proto_t *proto;
};

candy_cclosure_t *candy_cclosure_create(candy_gc_t *gc, candy_cfunc_t cfunc) {
  return NULL;
}

candy_sclosure_t *candy_sclosure_create(candy_gc_t *gc, candy_proto_t *proto) {
  candy_sclosure_t *closure = (candy_sclosure_t *)candy_gc_add(gc, TYPE_FUNC, sizeof(struct candy_sclosure));
  closure->proto = proto;
  return closure;
}

int candy_closure_delete(candy_sclosure_t *self, candy_gc_t *gc) {
  candy_gc_alloc(gc, self, sizeof(struct candy_sclosure), 0);
  return 0;
}

const candy_proto_t *candy_sclosure_get_proto(candy_sclosure_t *self) {
  return self->proto;
}
