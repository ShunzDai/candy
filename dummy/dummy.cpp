#include "candy.h"
#include "serial.h"
#include "src/candy_lib.h"
#include <stdint.h>
#include <stdio.h>

void func1() {
  printf("into %s\n", __FUNCTION__);
}

void func2(int arg1, float arg2, std::string arg3) {
  printf("into %s, arg1 %d, arg2 %.3f, arg3 '%s'\n", __FUNCTION__, arg1, arg2, arg3.data());
}

int func3() {
  printf("into %s\n", __FUNCTION__);
  return 314;
}

std::tuple<int, float, std::string> func4() {
  printf("into %s\n", __FUNCTION__);
  return {314, 3.14f, __FUNCTION__};
}

std::tuple<int, float, std::string> func5(bool arg1, int arg2, float arg3, std::string arg4) {
  printf("into %s, arg1 %d, arg2 %.3f, arg3 '%s'\n", __FUNCTION__, arg2, arg3, arg4.data());
  return {314, 3.14f, __FUNCTION__};
}

int main(int argc, char *argv[]) {
  candy::state sta;
  sta.add("__global__", candy::list(
    std::pair{"func1", func1},
    std::pair{"func2", func2},
    std::pair{"func3", func3},
    std::pair{"func4", func4},
    std::pair{"func5", func5}
  ));

  /* auto res1 = */ sta.call("func1");
  /* auto res2 = */ sta.call("func2", 314, 3.14f, "cpp str");
  /* auto res3 = */ sta.call<int>("func3");
  /* auto res4 = */ sta.call<int, float, std::string>("func4");
  /* auto res5 = */ sta.call<int, float, std::string>("func5", true, 314, 3.14f, "cpp str");

  // auto obuf = serial::pack(std::tuple{314, 3.14f, std::tuple{"hello", (uint8_t)0x55U}, "world", true});
  // for (auto e : obuf)
  // printf("%02X ", e);
  // printf("\n");
  // auto args = serial::unpack<int, float, std::string, uint8_t, std::string, bool>(obuf);
  // printf("arg1,%d,arg2,%f,arg3,%s,arg4,0x%02X,arg5,%s,arg6,%d,\n", std::get<0>(args), std::get<1>(args), std::get<2>(args).data(), std::get<3>(args), std::get<4>(args).data(), std::get<5>(args));
  return 0;
}
