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
#include "src/candy_proto.h"
#include "src/candy_object.h"
#include "src/candy_gc.h"
#include "src/candy_vector.h"

struct candy_proto {
  candy_object_t header;
  size_t size_inst;
  candy_inst_t *inst;
};

candy_proto_t *candy_proto_create(candy_gc_t *gc) {
  candy_proto_t *self = (candy_proto_t *)candy_gc_add(gc, TYPE_BLOCK, sizeof(struct candy_proto));
  // self->size_inst = candy_vector_size(cfg->inst);
  // self->inst = (candy_inst_t *)candy_vector_data(cfg->inst);
  self->inst = NULL;
  return self;
}

void candy_proto_delete(candy_proto_t *self, candy_gc_t *gc) {
  candy_gc_alloc(gc, self->inst, self->size_inst, 0);
  candy_gc_alloc(gc, self, sizeof(struct candy_proto), 0);
}

// int candy_proto_add_cnst(candy_proto_t *self, const candy_wrap_t *wrap) {
//   candy_array_append(self->cnst, wrap, 1);
//   return candy_array_size(self->cnst) - 1;
// }

// int candy_proto_add_inst(candy_proto_t *self, candy_inst_t inst) {
//   candy_array_append(self->inst, &inst, 1);
//   return candy_array_size(self->inst) - 1;
// }

candy_inst_t *candy_proto_get_inst(candy_proto_t *self) {
  return self->inst;
}
