#include "core/candy_lib.h"
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <float.h>

long strntol(const char nptr[], size_t size, char *endptr[], int base) {
  char buf[size + 1];
  memcpy(buf, nptr, size);
  buf[size] = '\0';
  long res = strtol(buf, endptr, base);
  if (endptr)
    *endptr = (char *)nptr + (*endptr - buf);
  return res;
}

double strntod(const char nptr[], size_t size, char *endptr[]) {
  char buf[size + 1];
  memcpy(buf, nptr, size);
  buf[size] = '\0';
  double res = strtod(buf, endptr);
  if (endptr)
    *endptr = (char *)nptr + (*endptr - buf);
  return res;
}
