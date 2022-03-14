#include "candy_platform.h"

int candy_platform_assert_error(const char *file, const char *func, int line, char *reason){
  printf("Assertion failed: %s, file %s, func %s, line %d\n", reason, file, func, line);
  while (1);
  return 0;
}
