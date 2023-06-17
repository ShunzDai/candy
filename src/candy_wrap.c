#include "src/candy_wrap.h"
#include "src/candy_lib.h"
#include <string.h>
#include <stdlib.h>
#include <inttypes.h>

const candy_wrap_t null = {{0}, {0, 0}};

int candy_wrap_fprint(const candy_wrap_t *self, FILE *out, int align, int (table_fprint)(const candy_wrap_t *, FILE *)) {
  switch (self->type) {
    case TYPE_NULL:
      return fprintf(out, "%*s", align, "NA");
    case TYPE_INTEGER:
      return fprintf(out, "%*" PRId64, align, *candy_wrap_get_integer(self));
    case TYPE_FLOAT:
      return fprintf(out, "%*f", align, *candy_wrap_get_float(self));
    case TYPE_STRING:
      return fprintf(out, "%*.*s", align, self->size, candy_wrap_get_string(self));
    case TYPE_USERDEF:
      return fprintf(out, "%*p", align, *candy_wrap_get_ud(self));
    case TYPE_BUILTIN:
      return fprintf(out, "%*p", align, *candy_wrap_get_builtin(self));
    case TYPE_TABLE:
      return table_fprint ? table_fprint(self, out) : fprintf(out, "%*p", align, self);
    default:
      assert(0);
      return 0;
  }
}

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
    size_t limit = next_power2(self->size + size);
    if (next_power2(self->size) < limit) {
      /* dst comes from either the stack space described by wrap::data or
         the heap space pointed to by wrap::data. if wrap::data is used to
         store the new heap space pointer directly, short data may be lost */
      void *temp = calloc(limit, candy_wrap_sizeof(self));
      memcpy(temp, dst, self->size * candy_wrap_sizeof(self));
      if (candy_wrap_check_ldata(self))
        free(dst);
      *(void **)&self->data = temp;
      dst = *(void **)&self->data;
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
