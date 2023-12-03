#include "src/candy_wrap.h"
#include "src/candy_lib.h"
#include <inttypes.h>

const candy_wrap_t null_wrap = {0};

int candy_wrap_fprint(const candy_wrap_t *self, FILE *out, int align) {
  switch (candy_wrap_get_type(self)) {
    case TYPE_NONE:
      return fprintf(out, "%*s", align, "NA");
    case TYPE_INTEGER:
      return fprintf(out, "%*" PRId64, align, candy_wrap_get_integer(self));
    case TYPE_FLOAT:
      return fprintf(out, "%*f", align, candy_wrap_get_float(self));
    // case TYPE_CFUNC:
    //   return fprintf(out, "%*p", align, candy_wrap_get_cfunc(self));
    default:
      assert(0);
      return 0;
  }
}
