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

struct candy_state {
  candy_gc_t gc;
  candy_vm_t vm;
};

static void *default_allocator(void *ptr, size_t old_size, size_t new_size, void *arg) {
  if (new_size)
    return realloc(ptr, new_size);
  free(ptr);
  return NULL;
}

candy_state_t *candy_state_create_allocator(candy_allocator_t alloc, void *arg) {
  candy_state_t *self = (candy_state_t *)alloc(NULL, 0, sizeof(struct candy_state), arg);
  candy_gc_init(&self->gc, alloc, arg);
  candy_vm_init(&self->vm, &self->gc);
  return self;
}

candy_state_t *candy_state_create(void) {
  return candy_state_create_allocator(default_allocator, NULL);
}

int candy_state_delete(candy_state_t *self) {
  candy_gc_deinit(&self->gc, (candy_handler_t[]) {
  #define CANDY_TYPE_HANDLER
  #include "src/candy_type.list"
  });
  candy_gc_alloc(&self->gc, self, sizeof(struct candy_state), 0);
  return 0;
}

int candy_dostream(candy_state_t *self, candy_reader_t reader, void *arg) {
  candy_object_t *out = candy_parse(&self->gc, reader, arg);
  if (candy_object_get_type(out) == TYPE_CHAR)
    printf("%.*s\n",
      (int)candy_array_size((candy_array_t *)out),
      (char *)candy_array_data((candy_array_t *)out)
    );
  // return candy_vm_execute(&self->vm, out);
  return 0;
}

int candy_dostring(candy_state_t *self, const char exp[]) {
  struct str_info info = {exp, strlen(exp), 0};
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
