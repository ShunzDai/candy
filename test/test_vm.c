#include "core/candy.h"
#include "core/candy_state.h"
#include <stdio.h>
#include <string.h>

static int failures;
static int checks;

static int verify(candy_state_t *state) {
  candy_integer_t actual = candy_state_to_integer(state, 0);
  candy_integer_t expected = candy_state_to_integer(state, 1);
  ++checks;
  if (actual != expected) {
    fprintf(stderr, "expected %lld, got %lld\n", (long long)expected, (long long)actual);
    ++failures;
  }
  return 0;
}

static void run(const char *source) {
  candy_state_t *state = candy_new_state_default();
  if (state == NULL) {
    ++failures;
    return;
  }
  if (candy_state_setglobal(state, "verify", verify) != CANDY_OK ||
      candy_dostring(state, source, strlen(source)) != CANDY_OK)
    ++failures;
  candy_close(state);
}

int main(int argc, char **argv) {
  const char *fibonacci =
    "def fibonacci(n)\n"
    "  if (n < 2)\n"
    "    return n\n"
    "  end\n"
    "  return fibonacci(n - 2) + fibonacci(n - 1)\n"
    "end\n";
  char source[1024];
  if (argc == 2 && strcmp(argv[1], "--fib40") == 0) {
    snprintf(source, sizeof(source), "%sverify(fibonacci(40), 102334155)\n", fibonacci);
    run(source);
    if (checks != 1)
      ++failures;
  } else {
    snprintf(source, sizeof(source), "%s%s", fibonacci,
      "verify(fibonacci(0), 0)\n"
      "verify(fibonacci(1), 1)\n"
      "verify(fibonacci(2), 1)\n"
      "verify(fibonacci(10), 55)\n"
      "verify(fibonacci(20), 6765)\n");
    run(source);
    run("def descend(n, result)\n"
        "  if (n < 1)\n"
        "    return result - 2\n"
        "  end\n"
        "  return descend(n - 1, result + 1)\n"
        "end\n"
        "verify(descend(20, 10), 28)\n");
    run("def decrement(n)\n"
        "  return n - 1\n"
        "end\n"
        "def count(n)\n"
        "  if (n < 1)\n"
        "    return 0\n"
        "  end\n"
        "  return count(decrement(n)) + 1\n"
        "end\n"
        "verify(count(20), 20)\n"
        "verify(decrement(10), 9)\n");
    run("def arithmetic(n, other)\n"
        "  verify(n - 2, 8)\n"
        "  verify(n - (2), 8)\n"
        "  verify(n - other, 7)\n"
        "  verify((n + other) - 2, 11)\n"
        "  if (other < 4)\n"
        "    verify(other, 3)\n"
        "  end\n"
        "  if (n < 4)\n"
        "    verify(0, 1)\n"
        "  end\n"
        "  return n\n"
        "end\n"
        "verify(arithmetic(10, 3), 10)\n");
    if (checks != 14)
      ++failures;
  }
  printf("VM regression: %d checks, %d failures\n", checks, failures);
  return failures == 0 ? 0 : 1;
}