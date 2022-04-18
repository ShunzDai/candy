#include "candy_platform.h"
#include <stdio.h>                        //uart redirect fputc
#include <string.h>                        //copy function
#include <stdarg.h>                        //va_start /vsnprintf /va_end function

void candy_platform_assert_error(const char *file, const char *func, int line, char *condition){
  printf("Assertion failed: %s\nfile %s, func %s, line %d\n", condition, file, func, line);
  while (1);
}
