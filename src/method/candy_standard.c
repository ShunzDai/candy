#include "candy_standard.h"
#include "src/struct/candy_object.h"

int candy_method_print(candy_object_t param){
  candy_assert(param != NULL);
  candy_pack_t *temp = (candy_pack_t *)candy_get_next(param);
  if (*temp == NULL)
      return -1;
  while (temp != NULL){
    switch (candy_pack_type(*temp)){
      case CANDY_TYPE_NONE:
        printf("None");
        break;
      case CANDY_TYPE_STRING:
        printf("%s", (char *)candy_pack_get_string(*temp).data);
        break;
      case CANDY_TYPE_INTEGER:
        printf("%ld", candy_pack_get_integer(*temp));
        break;
      case CANDY_TYPE_FLOAT:
        printf("%.5f", candy_pack_get_float(*temp));
        break;
      case CANDY_TYPE_BOOLEAN:
        printf("%s", candy_pack_get_boolean(*temp) ? "True" : "False");
        break;
      default:
        return -1;
    }
    temp = (candy_pack_t *)candy_get_next(*temp);
    if (*temp == NULL){
      printf("\n");
      break;
    }
    printf(" ");
  }
  return 0;
}

int candy_method_type(candy_object_t param){


  return 0;
}
