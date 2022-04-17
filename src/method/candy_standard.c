#include "candy_standard.h"
#include "src/struct/candy_object.h"

int candy_method_print(candy_object_t param){
  candy_assert(param != NULL);
  candy_pack_t *temp = (candy_pack_t *)candy_node_get_next(param);
  uint16_t size;
  if (*temp == NULL)
      return 0;
  while (temp != NULL){
    switch (candy_node_get_type(*temp)){
      case CANDY_TYPES_NONE:
        printf("None");
        break;
      case CANDY_TYPES_STRING:
        printf("%s", candy_pack_get_string(*temp, &size));
        break;
      case CANDY_TYPES_INTEGER:
        printf("%ld", candy_pack_get_integer(*temp));
        break;
      case CANDY_TYPES_FLOAT:
        printf("%.5f", candy_pack_get_float(*temp));
        break;
      case CANDY_TYPES_BOOLEAN:
        printf("%s", candy_pack_get_boolean(*temp) ? "True" : "False");
        break;
      default:
        return 0;
    }
    temp = (candy_pack_t *)candy_node_get_next(*temp);
    if (*temp == NULL){
      printf("\n");
      return 0;
    }
    printf(" ");
  }
  return 0;
}

int candy_method_type(candy_object_t param){


  return 1;
}
