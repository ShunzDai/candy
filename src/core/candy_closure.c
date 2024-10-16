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
#include "core/candy_closure.h"
#include "core/candy_object.h"
#include "core/candy_gc.h"

struct candy_cclosure {
  candy_object_t header;
  candy_object_t *gray;
  candy_cfunc_t cfunc;
};

struct candy_sclosure {
  candy_object_t header;
  candy_object_t *gray;
  candy_proto_t *proto;
};

candy_cclosure_t *candy_cclosure_create(candy_gc_t *gc, candy_exce_t *ctx, candy_cfunc_t cfunc) {
  candy_cclosure_t *self = (candy_cclosure_t *)candy_gc_add(gc, ctx, CANDY_TYPE_CCLSR, sizeof(struct candy_cclosure));
  self->gray = NULL;
  self->cfunc = cfunc;
  return self;
}

int candy_cclosure_delete(candy_cclosure_t *self, candy_gc_t *gc) {
  candy_gc_free(gc, self, sizeof(struct candy_cclosure));
  return 0;
}

candy_sclosure_t *candy_sclosure_create(candy_gc_t *gc, candy_exce_t *ctx, candy_proto_t *proto) {
  candy_sclosure_t *self = (candy_sclosure_t *)candy_gc_add(gc, ctx, CANDY_TYPE_SCLSR, sizeof(struct candy_sclosure));
  self->gray = NULL;
  self->proto = proto;
  return self;
}

int candy_sclosure_delete(candy_sclosure_t *self, candy_gc_t *gc) {
  candy_gc_free(gc, self, sizeof(struct candy_sclosure));
  return 0;
}

int candy_sclosure_colouring(candy_sclosure_t *self, candy_gc_t *gc) {
  return 0;
}

int candy_sclosure_diffusion(candy_sclosure_t *self, candy_gc_t *gc) {
  return -1;
}

const candy_proto_t *candy_sclosure_get_proto(candy_sclosure_t *self) {
  return self->proto;
}
