#include "src/candy_wrap.h"
#include "src/candy_buffer.h"
#include <string.h>

void *candy_wrap_get_data(const candy_wrap_t *self) {
  return candy_wrap_check_ldata(self, candy_wrap_sizeof(self)) ? (*(candy_buffer_t **)&self->data)->data : (void *)&self->data;
}

void candy_wrap_set_data(candy_wrap_t *self, candy_wraps_t type, const void *data, size_t size, size_t n) {
  self->type = type;
  self->size = size;
  memcpy(candy_wrap_check_ldata(self, n) ? ((*(candy_buffer_t **)&self->data) = candy_buffer_create(size, n, false))->data : (void *)&self->data, data, size * n);
}

void candy_wrap_init(candy_wrap_t *self) {
  memset(self, 0, sizeof(struct candy_wrap));
}

int candy_wrap_deinit(candy_wrap_t *self) {
  if (candy_wrap_check_ldata(self, candy_wrap_sizeof(self)))
    candy_buffer_delete((candy_buffer_t **)&self->data);
  memset(self, 0, sizeof(struct candy_wrap));
  return 0;
}
