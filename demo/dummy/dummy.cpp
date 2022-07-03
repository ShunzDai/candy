#include <stdint.h>
#include <stdio.h>

int add(int *a) {
  (*a)++;
  return *a;
}

int get(int *a) {
  return *a;
}

int main(int argc, char *argv[]) {
  int a = 0;
    printf("%d %d\n", get(&a), add(&a));
    return 0;
}
