#include "src/candy_wrap.h"
#include "src/candy_lib.h"
#include <string.h>
#include <stdlib.h>

const candy_wrap_t null = {{0, 0}, {0}};

void *candy_wrap_get_data(const candy_wrap_t *self) {
  return candy_wrap_check_ldata(self) ? *(void **)&self->data : (void *)&self->data;
}

void candy_wrap_set_data(candy_wrap_t *self, candy_wraps_t type, const void *data, size_t size) {
  self->type = (uint32_t)type;
  self->size = (uint32_t)size;
  if (candy_wrap_check_ldata(self))
    *(void **)&self->data = calloc(next_power2(size), candy_wrap_sizeof(self));
  if (data && size)
    memcpy(candy_wrap_get_data(self), data, size * candy_wrap_sizeof(self));
}

void candy_wrap_append(candy_wrap_t *self, const void *data, size_t size) {
  void *dst = candy_wrap_get_data(self);
  if ((self->size + size) * candy_wrap_sizeof(self) > sizeof(self->data)) {
    void *new = expand(dst, candy_wrap_sizeof(self), self->size, self->size + size);
    if (dst != new) {
      dst = new;
      if (candy_wrap_check_ldata(self))
        free(*(void **)&self->data);
      *(void **)&self->data = new;
    }
  }
  if (data && size)
    memcpy(dst + self->size * candy_wrap_sizeof(self), data, size * candy_wrap_sizeof(self));
  self->size += size;
}

void candy_wrap_init(candy_wrap_t *self) {
  memset(self, 0, sizeof(struct candy_wrap));
}

int candy_wrap_deinit(candy_wrap_t *self) {
  if (candy_wrap_check_ldata(self))
    free(*(void **)&self->data);
  memset(self, 0, sizeof(struct candy_wrap));
  return 0;
}
