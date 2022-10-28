#include "src/candy_state.h"
#include <stdlib.h>



struct candy_state *candy_state_create() {
  struct candy_state *s = (struct candy_state *)malloc(sizeof(struct candy_state));
  return s;
}

int candy_state_delete(struct candy_state **s) {
  free(*s);
  *s = NULL;
  return 0;
}
