#include "candy.h"
#include "mid_os.h"
#include <iostream>
#include <string.h>
#include <stdlib.h>
#include <mcheck.h>

int main(int argc, char *argv[]) {
  setenv("MALLOC_TRACE", "output", 1);
  mtrace();
  candy cdy;
  while (1) {
    std::string line;
    printf(">>> ");
    std::getline(std::cin, line);
    if (strcmp(line.c_str(), "q") == 0)
      break;
    cdy.dostring(line.c_str());
  }
  printf("quit\n");
}
