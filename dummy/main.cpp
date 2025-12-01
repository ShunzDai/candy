#include "candy.hpp"
#include <stdint.h>
#include <stdio.h>

static int test0(int a, int b) {
  return a + b;
}

int main(int argc, char *argv[]) {
  candy self;
  self.regist("test0", test0);
  return 0;
}
