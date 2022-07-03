#include "mid_os.h"
#include "src/core/candy_parser.h"
#include <stdio.h>

extern "C" void app_main(void) {
  printf("into %s\n", __func__);
  while (1) {
    auto tick = os::tick_us();
    auto parser = candy_parser_create("(((1 + (2 * 3)/(4 + 5))*(6 - 7) + (8 + 9) * 10)/11) - 12");
    printf("tick %llu\n", os::tick_us() - tick);
    candy_parser_delete(&parser);
    os::delay_ms(1000);
  }
}
