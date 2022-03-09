#include "candy_standard.h"
#include "src/struct/candy_object.h"

int candy_method_print(candy_object_t obj){
  printf("into %s\n", __FUNCTION__);
  
  return 0;
}
