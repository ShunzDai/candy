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

static candy_err_t _cclosure_delete(candy_cclosure_t *self, candy_gc_t *gc, void *arg) {
  candy_gc_free(gc, self, sizeof(candy_cclosure_t));
  return CANDY_OK;
}

static candy_err_t _cclosure_color(candy_cclosure_t *self, candy_gc_t *gc, void *arg) {
  return CANDY_OK;
}

static candy_err_t _cclosure_diffuse(candy_cclosure_t *self, candy_gc_t *gc, void *arg) {
  return CANDY_OK;
}

static candy_err_t _sclosure_delete(candy_sclosure_t *self, candy_gc_t *gc, void *arg) {
  candy_gc_free(gc, self, sizeof(candy_sclosure_t));
  return CANDY_OK;
}

static candy_err_t _sclosure_color(candy_sclosure_t *self, candy_gc_t *gc, void *arg) {
  return CANDY_OK;
}

static candy_err_t _sclosure_diffuse(candy_sclosure_t *self, candy_gc_t *gc, void *arg) {
  return CANDY_OK;
}

candy_cclosure_t *candy_cclosure_create(candy_gc_t *gc, candy_excep_t *ctx, candy_cfunc_t cfunc) {
  candy_cclosure_t *self = (candy_cclosure_t *)candy_gc_add(gc, ctx, CANDY_TYPE_CCLOS, sizeof(candy_cclosure_t));
  self->gray = NULL;
  self->cfunc = cfunc;
  return self;
}

candy_err_t candy_cclosure_handler(candy_cclosure_t *self, candy_gc_t *gc, candy_events_t evt, void *arg) {
  switch (evt) {
    case EVT_DELETE:  return _cclosure_delete(self, gc, arg);
    case EVT_COLOR:   return _cclosure_color(self, gc, arg);
    case EVT_DIFFUSE: return _cclosure_diffuse(self, gc, arg);
    default:          return CANDY_ERR;
  }
}

candy_sclosure_t *candy_sclosure_create(candy_gc_t *gc, candy_excep_t *ctx, candy_proto_t *proto) {
  candy_sclosure_t *self = (candy_sclosure_t *)candy_gc_add(gc, ctx, CANDY_TYPE_SCLOS, sizeof(candy_sclosure_t));
  self->gray = NULL;
  self->proto = proto;
  return self;
}

candy_err_t candy_sclosure_handler(candy_sclosure_t *self, candy_gc_t *gc, candy_events_t evt, void *arg) {
  switch (evt) {
    case EVT_DELETE:  return _sclosure_delete(self, gc, arg);
    case EVT_COLOR:   return _sclosure_color(self, gc, arg);
    case EVT_DIFFUSE: return _sclosure_diffuse(self, gc, arg);
    default:          return CANDY_ERR;
  }
}

const candy_proto_t *candy_sclosure_get_proto(candy_sclosure_t *self) {
  return self->proto;
}
