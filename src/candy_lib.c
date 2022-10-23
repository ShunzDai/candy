#include "candy_lib.h"

/* time33 hash map */
candy_hash_t candy_hash(char str[]) {
  candy_hash_t hash = 5381;
  while (*str)
    hash += (hash << 5) + (*str++);
  return (hash & 0x7FFFFFFF);
}
