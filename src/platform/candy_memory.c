#include "candy_memory.h"
#include <stdlib.h>

void *candy_malloc(size_t size){
  return malloc(size);
}

void candy_free(void *src){
  free(src);
}
