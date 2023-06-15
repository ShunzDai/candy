/**
  * Copyright 2022-2023 ShunzDai
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
#include "src/candy_state.h"
#include "src/candy_builtin.h"
#include "readline/readline.h"
#include <stdlib.h>

int main(int argc, const char *argv[]) {
  candy_state_t *state = candy_state_create(argc, argv, NULL);
  candy_add_builtin(state, candy_builtin_list, candy_builtin_size);
  if (argc > 1)
    return candy_dofile(state, argv[1]);
  printf("candy (%s, %s)\ntype 'exit()' to quit\n", __DATE__, __TIME__);
  while (1) {
    char *line = readline(">>> ");
    if (candy_dostring(state, line) != 0)
      printf("%s\n", candy_error(state));
    free(line);
    line = NULL;
  }
  /* it will never get here */
  abort();
}
