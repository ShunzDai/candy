/**
  * Copyright 2022-2025 ShunzDai
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
#include "core/candy_wrap.h"
#include "core/candy_gc.h"

struct candy_proto {
  candy_object_t header;

  candy_vector_t constv;
  candy_vector_t inst;
};

candy_proto_t *candy_proto_create(candy_gc_t *gc, candy_excep_t *ctx) {
  candy_proto_t *self = (candy_proto_t *)candy_gc_add(gc, ctx, CANDY_TYPE_PROTO, sizeof(candy_proto_t));
  candy_vector_init(&self->constv);
  candy_vector_init(&self->inst);
  return self;
}

candy_err_t candy_proto_delete(candy_proto_t *self, candy_gc_t *gc) {
  candy_vector_deinit(&self->inst, candy_gc_memory(gc), sizeof(candy_inst_t));
  candy_vector_deinit(&self->constv, candy_gc_memory(gc), sizeof(candy_wrap_t));
  candy_gc_free(gc, self, sizeof(candy_proto_t));
  return CANDY_OK;
}

candy_err_t candy_proto_color(candy_proto_t *self, candy_gc_t *gc) {
  candy_object_set_mark((candy_object_t *)self, MARK_DARK);
  return CANDY_OK;
}

candy_err_t candy_proto_diffuse(candy_proto_t *self, candy_gc_t *gc) {
  return CANDY_OK;
}

candy_vector_t *candy_proto_get_const(candy_proto_t *self) {
  return &self->constv;
}

candy_vector_t *candy_proto_get_inst(candy_proto_t *self) {
  return &self->inst;
}
