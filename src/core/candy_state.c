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
#include "core/candy_state.h"
#include "core/candy_object.h"
#include "core/candy_parser.h"
#include "core/candy_reader.h"
#include "core/candy_array.h"
#include "core/candy_table.h"
#include "core/candy_proto.h"
#include "core/candy_userdef.h"
#include "core/candy_closure.h"
#include "core/candy_vm.h"
#include "core/candy_gc.h"
#include <stdlib.h>
#include <string.h>

typedef struct candy_context candy_context_t;

struct candy_state {
  candy_object_t header;
  candy_object_t *gray;
  candy_gc_t *gc;
  candy_vm_t vm;
};

struct candy_context {
  candy_state_t state;
  candy_gc_t gc;
};

static void *default_allocator(void *ptr, size_t old_size, size_t new_size, void *arg) {
  if (new_size)
    return realloc(ptr, new_size);
  free(ptr);
  return NULL;
}

static size_t candy_state_size(candy_state_t *self) {
  return candy_state_is_main(self) ? sizeof(struct candy_context) : sizeof(struct candy_state);
}

static int candy_state_init(candy_state_t *self, candy_gc_t *gc) {
  self->gray = NULL;
  self->gc = gc;
  candy_vm_init(&self->vm);
  return 0;
}

static int candy_state_deinit(candy_state_t *self, candy_gc_t *gc) {
  candy_vm_deinit(&self->vm, gc);
  candy_gc_alloc(gc, self, candy_state_size(self), 0);
  return 0;
}

static int candy_state_colouring(candy_state_t *self, candy_gc_t *gc) {
  self->gray = candy_gc_gray_swap(gc, (candy_object_t *)self);
  candy_object_set_mark((candy_object_t *)self, MARK_GRAY);
  return 0;
}

static int candy_state_diffusion(candy_state_t *self, candy_gc_t *gc) {
  candy_gc_gray_swap(gc, self->gray);
  candy_object_set_mark((candy_object_t *)self, MARK_DARK);

  return 0;
}

static int _event_delete(candy_object_t *self, candy_gc_t *gc) {
  if (candy_object_get_mask(self) & MASK_ARRAY)
    return candy_array_delete((candy_array_t *)self, gc);
  switch (candy_object_get_type(self)) {
    case CANDY_EXTD_CCLSR: return candy_cclosure_delete((candy_cclosure_t *)self, gc);
    case CANDY_EXTD_SCLSR: return candy_sclosure_delete((candy_sclosure_t *)self, gc);
    case CANDY_EXTD_HEAVY: return candy_userdef_delete((candy_userdef_t *)self, gc);
    case CANDY_BASE_TABLE: return candy_table_delete((candy_table_t *)self, gc);
    case CANDY_BASE_PROTO: return candy_proto_delete((candy_proto_t *)self, gc);
    case CANDY_BASE_STATE: return candy_state_deinit((candy_state_t *)self, gc);
    default:               return -1;
  }
}

static int _event_colouring(candy_object_t *self, candy_gc_t *gc) {
  switch (candy_object_get_type(self)) {
    case CANDY_EXTD_CCLSR: return -1;
    case CANDY_EXTD_SCLSR: return candy_sclosure_colouring((candy_sclosure_t *)self, gc);
    case CANDY_EXTD_HEAVY: return -1;
    case CANDY_BASE_TABLE: return -1;
    case CANDY_BASE_PROTO: return candy_proto_colouring((candy_proto_t *)self, gc);
    case CANDY_BASE_STATE: return candy_state_colouring((candy_state_t *)self, gc);
    default:               return -1;
  }
}

static int _event_diffusion(candy_object_t *self, candy_gc_t *gc) {
  switch (candy_object_get_type(self)) {
    case CANDY_EXTD_CCLSR: return -1;
    case CANDY_EXTD_SCLSR: return candy_sclosure_diffusion((candy_sclosure_t *)self, gc);
    case CANDY_EXTD_HEAVY: return -1;
    case CANDY_BASE_TABLE: return -1;
    case CANDY_BASE_PROTO: return candy_proto_diffusion((candy_proto_t *)self, gc);
    case CANDY_BASE_STATE: return candy_state_diffusion((candy_state_t *)self, gc);
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

candy_state_t *candy_state_create(candy_allocator_t alloc, void *arg) {
  candy_gc_t gc;
  candy_gc_init(&gc, _event_handler, alloc, arg);
  candy_context_t *ctx = (candy_context_t *)candy_gc_add(&gc, CANDY_BASE_STATE, sizeof(struct candy_context));
  memcpy(&ctx->gc, &gc, sizeof(struct candy_gc));
  candy_gc_move(&ctx->gc, GC_MV_MAIN);
  candy_state_t *co = &ctx->state;
  candy_state_init(co, &ctx->gc);
  return co;
}

candy_state_t *candy_state_create_default(void) {
  return candy_state_create(default_allocator, NULL);
}

candy_state_t *candy_state_create_coroutine(candy_state_t *self) {
  candy_state_t *co = (candy_state_t *)candy_gc_add(self->gc, CANDY_BASE_STATE, sizeof(struct candy_state));
  candy_state_init(co, self->gc);
  return co;
}

int candy_state_delete(candy_state_t *self) {
  if (candy_state_is_main(self)) {
    candy_gc_deinit(self->gc);
  }
  return 0;
}

int candy_state_dostream(candy_state_t *self, candy_reader_t reader, void *arg) {
  candy_object_t *out = candy_parse(self->gc, reader, arg);
  if (candy_object_get_type(out) == CANDY_BASE_CHAR)
    printf("%.*s\n",
      (int)candy_array_size((candy_array_t *)out),
      (char *)candy_array_data((candy_array_t *)out)
    );
  // return candy_vm_execute(&self->vm, out);
  return candy_gc_full(self->gc);
}

int candy_state_dostring(candy_state_t *self, const char exp[]) {
  struct str_info info = {exp, strlen(exp), 0};
  int res = candy_state_dostream(self, string_reader, &info);
  return res;
}

int candy_state_dofile(candy_state_t *self, const char name[]) {
  FILE *f = fopen(name, "r");
  if (f == NULL)
    return perror(NULL), -1;
  struct file_info info = {f};
  int res = candy_state_dostream(self, file_reader, &info);
  fclose(f);
  return res;
}

bool candy_state_is_main(candy_state_t *self) {
  return candy_gc_main(self->gc) == (candy_object_t *)self;
}
