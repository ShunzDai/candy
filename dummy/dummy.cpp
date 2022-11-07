#include "candy.h"
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <mcheck.h>

void cfunc1() {
  printf("into %s\n", __FUNCTION__);
}

void cfunc2(int arg1, float arg2, std::string arg3) {
  printf("into %s, arg1 %d, arg2 %.3f, arg3 '%s'\n", __FUNCTION__, arg1, arg2, arg3.data());
}

int cfunc3() {
  printf("into %s\n", __FUNCTION__);
  return 314;
}

std::tuple<int, float, std::string> cfunc4() {
  printf("into %s\n", __FUNCTION__);
  return {314, 3.14f, __FUNCTION__};
}

std::tuple<int, float, std::string> cfunc5(int arg1, float arg2, std::string arg3) {
  printf("into %s, arg1 %d, arg2 %.3f, arg3 '%s'\n", __FUNCTION__, arg1, arg2, arg3.data());
  return {314, 3.14f, __FUNCTION__};
}

int main(int argc, char *argv[]) {
  setenv("MALLOC_TRACE", "output", 1);
  mtrace();
  candy cdy;
  cdy.regist("__global__",
    std::pair{"cfunc1", cfunc1},
    std::pair{"cfunc2", cfunc2},
    std::pair{"cfunc3", cfunc3},
    std::pair{"cfunc4", cfunc4},
    std::pair{"cfunc5", cfunc5}
  );
  /* auto res1 = */ cdy.call("cfunc1");
  /* auto res2 = */ cdy.call("cfunc2", 314, 3.14f, "str from cpp");
  /* auto res3 = */ cdy.call<int>("cfunc3");
  /* auto res4 = */ cdy.call<int, float, std::string>("cfunc4");
  /* auto res5 = */ cdy.call<int, float, std::string>("cfunc5", 314, 3.14f, "str from cpp");
  return 0;
}
