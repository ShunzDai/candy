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
#include "core/candy_proto.h"
#include "core/candy_object.h"
#include "core/candy_vector.h"
#include "core/candy_gc.h"

struct candy_proto {
  candy_object_t header;
  size_t size_inst;
  candy_inst_t *inst;
};

candy_proto_t *candy_proto_create(candy_gc_t *gc, candy_exce_t *ctx) {
  candy_proto_t *self = (candy_proto_t *)candy_gc_add(gc, ctx, CANDY_TYPE_PROTO, sizeof(struct candy_proto));
  self->size_inst = 0;
  // self->inst = (candy_inst_t *)candy_vector_data(cfg->inst);
  self->inst = NULL;
  return self;
}

int candy_proto_delete(candy_proto_t *self, candy_gc_t *gc) {
  candy_gc_free(gc, self->inst, self->size_inst);
  candy_gc_free(gc, self, sizeof(struct candy_proto));
  return 0;
}

int candy_proto_colouring(candy_proto_t *self, candy_gc_t *gc) {
  candy_object_set_mark((candy_object_t *)self, MARK_DARK);
  return 0;
}

int candy_proto_diffusion(candy_proto_t *self, candy_gc_t *gc) {
  return 0;
}

candy_inst_t *candy_proto_get_inst(candy_proto_t *self) {
  return self->inst;
}
