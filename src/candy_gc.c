#include "candy_gc.h"
#include "src/candy_object.h"
#include <stdlib.h>

static candy_object_t *candy_gc_add_node(candy_object_t **self, candy_wraps_t type, int size) {
  candy_object_t *obj = (candy_object_t *)calloc(1, size);
  candy_wrap_set_type((candy_wrap_t *)obj, type);
  candy_wrap_set_object((candy_wrap_t *)obj, *self);
  *self = obj;
  return obj;
}

void candy_gc_del_node(candy_object_t **self) {
  candy_object_t *obj = *self;
  *self = *(candy_object_t **)candy_wrap_data((candy_wrap_t *)obj);
  switch (candy_wrap_get_type((candy_wrap_t *)obj)) {
    case TYPE_CHAR:
      free(((candy_array_t *)obj)->data);
      break;
    default:
      break;
  }
  free(obj);
}

int candy_gc_init(candy_gc_t *self) {
  self->pool = NULL;
  self->root = NULL;
  return 0;
}

int candy_gc_deinit(candy_gc_t *self) {
  while (self->root) {
    candy_gc_del_node(&self->root);
  }
  while (self->pool) {
    candy_gc_del_node(&self->pool);
  }
  return 0;
}

candy_object_t *candy_gc_add_pool(candy_gc_t *self, candy_wraps_t type, int size) {
  return candy_gc_add_node(&self->pool, type, size);
}

candy_object_t *candy_gc_add_root(candy_gc_t *self, candy_wraps_t type, int size) {
  return candy_gc_add_node(&self->root, type, size);
}

// int iterator(candy_wrap_t *self, int(*cb)(candy_wrap_t *)) {
//   for (candy_wrap_t *node = self; node; node = *(candy_wrap_t **)candy_wrap_data(node)) {
//     switch (candy_wrap_get_type(node)) {
//       case TYPE_TUPLE:

//         break;
//       default:
//         break;
//     }
//   }
// }

void mark_tuple(candy_object_t *self) {

}

// int candy_gc_mark(candy_gc_t *self) {
//   for (candy_object_t *node = self->root; node; node = *(candy_object_t **)candy_wrap_data((candy_wrap_t *)node)) {
//     switch (candy_wrap_get_type((candy_wrap_t *)node)) {
//       case TYPE_TUPLE:
//         mark_tuple(node);
//         break;
//       default:
//         break;
//     }
//   }
//   return 0;
// }

int candy_gc_sweep(candy_gc_t *self) {

  return 0;
}

int candy_gc_run(candy_gc_t *self) {
  // candy_gc_mark(self);
  candy_gc_sweep(self);
  return 0;
}
