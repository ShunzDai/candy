#include "candy_array.h"
#include "src/candy_object.h"
#include "src/candy_gc.h"
#include "src/candy_lib.h"
#include <string.h>
#include <stdlib.h>

candy_array_t *candy_array_new(candy_gc_t *gc, candy_wraps_t type, void *data, size_t size) {
  candy_array_t *self = (candy_array_t *)candy_gc_add_pool(gc, type, sizeof(struct candy_array));
  self->size = 0;
  self->data = NULL;
  if (size)
    candy_array_append(self, data, size);
  return self;
}

size_t candy_array_size(candy_array_t *self) {
  return self->size;
}

void *candy_array_data(candy_array_t *self) {
  return self->data;
}

size_t candy_array_capacity(candy_array_t *self) {
  return next_power2(self->size);
}

void candy_array_resize(candy_array_t *self, size_t size) {
  size_t sz = candy_array_size(self);
  if (size > sz)
    candy_array_append(self, NULL, size - sz);
  else
    self->size = size;
}

int candy_array_append(candy_array_t *self, void *data, size_t size) {
  size_t cap = candy_array_capacity(self);
  size_t sz = candy_array_size(self);
  if (cap - sz >= size) {
    if (data && size) {
      memcpy(self->data + sz * candy_type_sizeof(self->obj.type), data, size * candy_type_sizeof(self->obj.type));
    }
  }
  else {
    void *temp = calloc(next_power2(sz + size), candy_type_sizeof(self->obj.type));
    if (sz)
      memcpy(temp, self->data, sz * candy_type_sizeof(self->obj.type));
    if (data && size)
      memcpy(temp + sz * candy_type_sizeof(self->obj.type), data, size * candy_type_sizeof(self->obj.type));
    if (self->data)
      free(self->data);
    self->data = temp;
  }
  sz += size;
  self->size = sz;
  return 0;
}
