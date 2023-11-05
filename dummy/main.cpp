#include "src/candy_lib.h"
#include <stdint.h>
#include <stdio.h>

struct Test {
  int8_t a : 4;
  int8_t b : 4;
};

int main(int argc, char *argv[]) {
  Test t = {-4, 5};
  printf("%02X\n", *(uint8_t*)&t);
  return 0;
}
