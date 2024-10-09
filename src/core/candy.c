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
#include "core/candy.h"
#include "core/candy_object.h"
#include "core/candy_reader.h"
#include "core/candy_gc.h"
#include "core/candy_array.h"
#include "core/candy_table.h"
#include "core/candy_proto.h"
#include "core/candy_closure.h"
#include "core/candy_userdef.h"
#include "core/candy_state.h"
#include <stdlib.h>

static void *default_allocator(void *prev, size_t prev_size, size_t next_size, void *arg) {
  if (next_size)
    return realloc(prev, next_size);
  free(prev);
  return NULL;
}

static int _event_delete(candy_object_t *self, candy_gc_t *gc) {
  if (candy_object_get_mask(self) & MASK_ARRAY)
    return candy_array_delete((candy_array_t *)self, gc);
  switch (candy_object_get_type(self)) {
    case CANDY_TYPE_CCLSR: return candy_cclosure_delete((candy_cclosure_t *)self, gc);
    case CANDY_TYPE_SCLSR: return candy_sclosure_delete((candy_sclosure_t *)self, gc);
    case CANDY_TYPE_UDHVY: return candy_userdef_delete((candy_userdef_t *)self, gc);
    case CANDY_TYPE_TABLE: return candy_table_delete((candy_table_t *)self, gc);
    case CANDY_TYPE_PROTO: return candy_proto_delete((candy_proto_t *)self, gc);
    case CANDY_TYPE_STATE: return candy_state_delete((candy_state_t *)self, gc);
    default:               return -1;
  }
}

static int _event_colouring(candy_object_t *self, candy_gc_t *gc) {
  switch (candy_object_get_type(self)) {
    case CANDY_TYPE_CCLSR: return -1;
    case CANDY_TYPE_SCLSR: return candy_sclosure_colouring((candy_sclosure_t *)self, gc);
    case CANDY_TYPE_UDHVY: return -1;
    case CANDY_TYPE_TABLE: return -1;
    case CANDY_TYPE_PROTO: return candy_proto_colouring((candy_proto_t *)self, gc);
    case CANDY_TYPE_STATE: return candy_state_colouring((candy_state_t *)self, gc);
    default:               return -1;
  }
}

static int _event_diffusion(candy_object_t *self, candy_gc_t *gc) {
  switch (candy_object_get_type(self)) {
    case CANDY_TYPE_CCLSR: return -1;
    case CANDY_TYPE_SCLSR: return candy_sclosure_diffusion((candy_sclosure_t *)self, gc);
    case CANDY_TYPE_UDHVY: return -1;
    case CANDY_TYPE_TABLE: return -1;
    case CANDY_TYPE_PROTO: return candy_proto_diffusion((candy_proto_t *)self, gc);
    case CANDY_TYPE_STATE: return candy_state_diffusion((candy_state_t *)self, gc);
    default:               return -1;
  }
}

static int _event_handler(candy_object_t *self, candy_gc_t *gc, candy_events_t evt) {
  switch (evt) {
    case EVT_DELETE:    return _event_delete(self, gc);
    case EVT_COLOURING: return _event_colouring(self, gc);
    case EVT_DIFFUSION: return _event_diffusion(self, gc);
    default:            return -1;
  }
}

candy_state_t *candy_new_state(candy_allocator_t alloc, void *arg) {
  candy_gc_t gc;
  candy_gc_init(&gc, _event_handler, alloc, arg);
  return candy_state_create(&gc);
}

candy_state_t *candy_new_state_default(void) {
  return candy_new_state(default_allocator, NULL);
}

candy_state_t *candy_new_coroutine(candy_state_t *self) {
  return candy_state_create_coroutine(self);
}

int candy_close(candy_state_t *self) {
  return candy_state_close(self);
}

int candy_dostream(candy_state_t *self, candy_reader_t reader, void *arg) {
  return candy_state_dostream(self, reader, arg);
}

int candy_dostring(candy_state_t *self, const char exp[], size_t size) {
  struct str_info info = {exp, size, 0};
  int res = candy_dostream(self, string_reader, &info);
  return res;
}

int candy_dofile(candy_state_t *self, const char name[]) {
  FILE *f = fopen(name, "r");
  if (f == NULL)
    return perror(NULL), -1;
  struct file_info info = {f};
  int res = candy_dostream(self, file_reader, &info);
  fclose(f);
  return res;
}
