#include "candy_standard.h"
#include "src/struct/candy_object.h"

candy_register_t candy_stdlib[] = {
  {"print", candy_method_print},
  {"type", candy_method_type},
  {NULL, NULL},
};

int candy_method_print(candy_object_t obj){
  printf("into %s\n", __FUNCTION__);

  return 0;
}

int candy_method_type(candy_object_t obj){


  return 0;
}
