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

static const char TAG[] = "candy";

static void *_default_allocator(void *prev, size_t prev_size, size_t next_size, void *arg) {
  (void)prev_size, (void)arg;
  if (next_size)
    return realloc(prev, next_size);
  free(prev);
  return NULL;
}

static candy_err_t _event_handler(candy_object_t *self, candy_gc_t *gc, candy_events_t evt, void *arg) {
  candy_logd(TAG, "%s type %s", candy_event_str(evt), candy_type_str(candy_object_type(self)));
  if (candy_object_mask(self) & MASK_ARRAY)
    return candy_array_handler((candy_array_t *)self, gc, evt, arg);
  switch (candy_object_type(self)) {
    case CANDY_TYPE_CCLOS: return candy_cclosure_handler((candy_cclosure_t *)self, gc, evt, arg);
    case CANDY_TYPE_SCLOS: return candy_sclosure_handler((candy_sclosure_t *)self, gc, evt, arg);
    case CANDY_TYPE_TABLE: return -1;
    case CANDY_TYPE_PROTO: return candy_proto_handler((candy_proto_t *)self, gc, evt, arg);
    case CANDY_TYPE_STATE: return candy_state_handler((candy_state_t *)self, gc, evt, arg);
    default:               return -1;
  }
}

candy_state_t *candy_new_state(candy_allocator_t alloc, void *arg) {
  return candy_state_create(_event_handler, alloc, arg);
}

candy_state_t *candy_new_state_default(void) {
  return candy_new_state(_default_allocator, NULL);
}

candy_state_t *candy_new_coroutine(candy_state_t *self) {
  return candy_state_create_coroutine(self);
}

candy_err_t candy_close(candy_state_t *self) {
  return candy_state_close(self);
}

candy_err_t candy_dostream(candy_state_t *self, candy_reader_t reader, void *arg) {
  return candy_state_dostream(self, reader, arg);
}

candy_err_t candy_dostring(candy_state_t *self, const char exp[], size_t size) {
  struct str_info info = {exp, size, 0};
  int res = candy_dostream(self, string_reader, &info);
  return res;
}

candy_err_t candy_dofile(candy_state_t *self, const char name[]) {
  FILE *f = fopen(name, "r");
  if (f == NULL)
    return perror(NULL), -1;
  struct file_info info = {f};
  int res = candy_dostream(self, file_reader, &info);
  fclose(f);
  return res;
}
