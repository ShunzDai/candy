#include <stdio.h>

extern "C" void app_main(void) {
  printf("into %s\n", __func__);
}
