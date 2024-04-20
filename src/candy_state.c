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
#include "src/candy_state.h"
#include "src/candy_object.h"
#include "src/candy_parser.h"
#include "src/candy_reader.h"
#include "src/candy_array.h"
#include "src/candy_table.h"
#include "src/candy_proto.h"
#include "src/candy_closure.h"
#include "src/candy_vm.h"
#include "src/candy_gc.h"
#include <stdlib.h>
#include <string.h>

typedef struct candy_context candy_context_t;

struct candy_state {
  candy_object_t header;
  candy_vm_t vm;
  candy_context_t *ctx;
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

static int candy_coroutine_init(candy_state_t *self, candy_context_t *ctx) {
  candy_vm_init(&self->vm);
  self->ctx = ctx;
  return 0;
}

static int candy_coroutine_delete(candy_state_t *self, candy_gc_t *gc) {
  candy_vm_deinit(&self->vm, gc);
  candy_gc_alloc(gc, self, candy_state_size(self), 0);
  return 0;
}

candy_state_t *candy_state_create(candy_allocator_t alloc, void *arg) {
  candy_context_t *ctx = (candy_context_t *)alloc(NULL, 0, sizeof(struct candy_context), arg);
  candy_gc_init(&ctx->gc, alloc, arg);
  candy_state_t *self = &ctx->state;
  candy_object_set_next((candy_object_t *)self, NULL);
  candy_object_set_type((candy_object_t *)self, TYPE_STATE);
  candy_coroutine_init(self, ctx);
  return self;
}

candy_state_t *candy_state_create_default(void) {
  return candy_state_create(default_allocator, NULL);
}

candy_state_t *candy_state_create_coroutine(candy_state_t *self) {
  candy_context_t *ctx = self->ctx;
  candy_state_t *co = (candy_state_t *)candy_gc_add(&ctx->gc, TYPE_STATE, sizeof(struct candy_state));
  candy_coroutine_init(co, ctx);
  return co;
}

int candy_state_delete(candy_state_t *self) {
  candy_context_t *ctx = self->ctx;
  if (candy_state_is_main(self)) {
    candy_gc_deinit(&ctx->gc, (candy_handler_t[]) {
      #define CANDY_TYPE_HANDLER
      #include "src/candy_type.list"
    });
  }
  candy_coroutine_delete(self, &ctx->gc);
  return 0;
}

int candy_state_dostream(candy_state_t *self, candy_reader_t reader, void *arg) {
  candy_context_t *ctx = self->ctx;
  candy_object_t *out = candy_parse(&ctx->gc, reader, arg);
  if (candy_object_get_type(out) == TYPE_CHAR)
    printf("%.*s\n",
      (int)candy_array_size((candy_array_t *)out),
      (char *)candy_array_data((candy_array_t *)out)
    );
  // return candy_vm_execute(&self->vm, out);
  return 0;
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
  candy_context_t *ctx = self->ctx;
  return self == &ctx->state;
}
