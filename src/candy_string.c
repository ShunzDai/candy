#include "candy_gc.h"
#include "src/candy_wrap.h"
#include <stdlib.h>

static candy_wrap_t *candy_gc_add_node(candy_wrap_t **self, candy_wraps_t type, int size) {
  candy_wrap_t *obj = (candy_wrap_t *)calloc(1, size);
  obj->type = type;
  *(candy_wrap_t **)candy_wrap_get_sdata(obj) = *self;
  *self = obj;
  return obj;
}

void candy_gc_del_node(candy_wrap_t **self) {
  candy_wrap_t *obj = *self;
  *self = *(candy_wrap_t **)candy_wrap_get_sdata(obj);
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
  return 0;
}

candy_wrap_t *candy_gc_add_pool(candy_gc_t *self, candy_wraps_t type, int size) {
  return candy_gc_add_node(&self->pool, type, size);
}

candy_wrap_t *candy_gc_add_root(candy_gc_t *self, candy_wraps_t type, int size) {
  return candy_gc_add_node(&self->root, type, size);
}
