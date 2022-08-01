#include <stdint.h>
#include <stdio.h>

int a() {
  printf("into %s\n", __FUNCTION__);
  return 0;
}

int b() {
  printf("into %s\n", __FUNCTION__);
  return 1;
}

int main(int argc, char *argv[]) {
  if (a() || b())
    printf("into if\n");
  else
    printf("into else\n");
  return 0;
}
