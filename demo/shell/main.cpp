/**
  * Copyright 2022 ShunzDai
  *
  * Licensed under the Apache License, Version 2.0 (the "License");
  * you may not use this file except in compliance with the License.
  * You may obtain a copy of the License at
  *
  *     http://www.apache.org/licenses/LICENSE-2.0
  *
  * Unless required by applicable law or agreed to in writing, software
  * distributed under the License is distributed on an "AS IS" BASIS,
  * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  * See the License for the specific language governing permissions and
  * limitations under the License.
  */
#include "candy.h"
#include "mid_os.h"
#include <iostream>
#include <string.h>

int main(int argc, char *argv[]) {
  candy cdy;
  printf("candy (%s, %s)\ntype 'q' to quit\n", __DATE__, __TIME__);
  while (1) {
    std::string line;
    printf(">>> ");
    std::getline(std::cin, line);
    if (strcmp(line.c_str(), "q") == 0)
      break;
    cdy.dostring(line.c_str());
  }
}
