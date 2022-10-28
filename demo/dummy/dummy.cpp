#include "candy.h"
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <mcheck.h>

void func1() {
  printf("into %s\n", __FUNCTION__);
}

void func2(int arg1, float arg2, std::string arg3) {
  printf("into %s, arg1 %d, arg2 %.3f, arg3 '%s'\n", __FUNCTION__, arg1, arg2, arg3.data());
}

std::string func3() {
  printf("into %s\n", __FUNCTION__);
  return "test3 return";
}

std::tuple<int, float, std::string> func4() {
  printf("into %s\n", __FUNCTION__);
  return {314, 3.14f, "test3 return"};
}

std::tuple<int, float, std::string> func5(int arg1, float arg2, std::string arg3) {
  printf("into %s, arg1 %d, arg2 %.3f, arg3 '%s'\n", __FUNCTION__, arg1, arg2, arg3.data());
  return {314, 3.14f, "test3 return"};
}

int main(int argc, char *argv[]) {
  setenv("MALLOC_TRACE", "output", 1);
  mtrace();
  candy cdy;
  /* auto res1 = */ cdy.call("func1");
  /* auto res2 = */ cdy.call("func2", 314, 3.14f, "str from cpp");
  /* auto res3 = */ cdy.call<std::string>("func3", 314, 3.14f, "str from cpp");
  /* auto res4 = */ cdy.call<int, float, std::string>("func4");
  /* auto res5 = */ cdy.call<int, float, std::string>("func5", 314, 3.14f, "str from cpp");
  cdy.call(func1);
  cdy.call(func2);
  cdy.call(func3);
  cdy.call(func4);
  cdy.call(func5);
  return 0;
}
