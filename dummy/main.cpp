#include "src/candy_lib.h"
#include <stdint.h>
#include <stdio.h>
#include <string>

struct Test {
  int8_t a : 4;
  int8_t b : 4;
};

int main(int argc, char *argv[]) {
  std::basic_string<uint8_t> s;
  s.reserve(222);
  for (size_t sz = 0; sz < 100; ++sz)
    printf("sz %zu cap %zu\n", sz, next_power2(sz));
  return 0;
}
