/**
  * Copyright 2022-2024 ShunzDai
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
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <stdatomic.h>

static const char head[] =
"candy " CANDY_VERSION_STR ", " CANDY_ENV ", " CANDY_GIT_DESCRIBE "\n" \
__DATE__ " " __TIME__ ", \
" CANDY_COMPILER_ID "-" CANDY_COMPILER_VERSION " on \
" CANDY_SYSTEM_NAME "-" CANDY_SYSTEM_VERSION "\n" \
"type 'Ctrl+C' to quit\n";

static _Atomic(bool) _quit = false;

static int stream_reader(char buffer[], const size_t max_len, void *arg) {
  int *ch = (int *)arg;
  if (*ch == '\n')
    fwrite("> ", 1, 2, stdout);
  while ((*ch = getchar()) < 0) {
    bool expected = false;
    if (!atomic_compare_exchange_strong(&_quit, &expected, false))
      return -1;
  }
  buffer[0] = *ch;
  return 1;
}

static void handle_sig(int sig) {
  bool expected = false;
  atomic_compare_exchange_strong(&_quit, &expected, true);
}

int main(int argc, const char *argv[]) {
  candy_state_t *state = candy_state_create_default();
  if (argc > 1) {
    candy_state_dofile(state, argv[1]);
    candy_state_delete(state);
    return 0;
  }
  fcntl(fileno(stdin), F_SETFL, O_NONBLOCK);
  signal(SIGINT, handle_sig);
  printf(head);
  bool expected = false;
  while (atomic_compare_exchange_strong(&_quit, &expected, false)) {
    int ch = '\n';
    if (candy_state_dostream(state, stream_reader, &ch) < 0)
      (void)0;
  }
  candy_state_delete(state);
  return 0;
}
