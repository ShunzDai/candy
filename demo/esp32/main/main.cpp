#include "mid_os.h"
#include "src/core/candy_parser.h"
#include <stdio.h>

os::timer_t t([](os::timer_t *timer) {
  auto tick = os::tick_us();
  auto parser = candy_parser_create("(((1 + (2 * 3)/(4 + 5))*(6 - 7) + (8 + 9) * 10)/11) - 12");
  printf("tick %llu us\n", os::tick_us() - tick);
  candy_parser_delete(&parser);
  timer->start(1000);
}, nullptr, "timer");

extern "C" void app_main(void) {
  printf("into %s\n", __func__);
  t.start(1000);
}
