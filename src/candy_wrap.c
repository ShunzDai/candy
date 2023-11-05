#include "src/candy_wrap.h"
#include "src/candy_lib.h"
#include <string.h>
#include <stdlib.h>
#include <inttypes.h>

const candy_wrap_t null = {0};

int candy_wrap_fprint(const candy_wrap_t *self, FILE *out, int align) {
  switch (candy_wrap_type(self)) {
    case TYPE_NULL:
      return fprintf(out, "%*s", align, "NA");
    case TYPE_INTEGER:
      return fprintf(out, "%*" PRId64, align, candy_wrap_get_integer(self));
    case TYPE_FLOAT:
      return fprintf(out, "%*f", align, candy_wrap_get_float(self));
    // case TYPE_STRING:
    //   return fprintf(out, "%*.*s", align, candy_wrap_size(self), candy_wrap_get_string(self));
    case TYPE_USERDEF:
      return fprintf(out, "%*p", align, candy_wrap_get_ud(self));
    case TYPE_CFUNC:
      return fprintf(out, "%*p", align, *candy_wrap_get_cfunc(self));
    default:
      assert(0);
      return 0;
  }
}
