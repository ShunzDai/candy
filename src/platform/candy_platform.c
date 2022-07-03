#include "candy_platform.h"
#include <stdio.h>                        //uart redirect fputc
#include <string.h>                        //copy function
#include <stdarg.h>                        //va_start /vsnprintf /va_end function
#include <stdlib.h>

void candy_platform_assert_error(const char *file, int line, const char *func, char *format, ...) {
  va_list ap;
  printf("%s:%d: In function '%s':\n", file, line, func);
  va_start(ap, format);
  vprintf(format, ap);
  va_end(ap);
  printf("\n");
  abort();
}
