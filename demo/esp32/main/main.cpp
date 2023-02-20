#include "candy.h"
#include "mid_os.h"
#include "esp_system.h"
#include <stdio.h>
#include <iostream>
#include <string.h>

extern "C" void app_main(void) {
  candy::state sta;
  std::string line;
  printf(">>> ");
  while (1) {
    int ch = getchar();
    if (ch < 0) {
      os::delay_ms(10);
      continue;
    }
    else if (ch != '\n') {
      line += (char)ch;
      printf("%c", ch);
      os::delay_ms(10);
      continue;
    }
    else if (strcmp(line.c_str(), "q") == 0)
      break;
    printf("%c\n", ch);
    sta.dostring(line.c_str());
    line.erase();
    printf(">>> ");
  }
  esp_restart();
}
