#include "candy_standard.h"
#include "src/struct/candy_object.h"

int candy_std_print(candy_object_t param) {
  // candy_assert(param != NULL);
  // candy_wrap_t *temp = &param->next;
  // if (*temp == NULL)
  //     return -1;
  // while (temp != NULL) {
  //   switch (candy_wrap_type(*temp)) {
  //     case CANDY_WRAP_NONE:
  //       printf("None");
  //       break;
  //     case CANDY_WRAP_STRING:
  //       printf("%s", (char *)candy_wrap_get_string(*temp).data);
  //       break;
  //     case CANDY_WRAP_INTEGER:
  //       printf("%ld", candy_wrap_get_integer(*temp));
  //       break;
  //     case CANDY_WRAP_FLOAT:
  //       printf("%.5f", candy_wrap_get_float(*temp));
  //       break;
  //     case CANDY_WRAP_BOOLEAN:
  //       printf("%s", candy_wrap_get_boolean(*temp) ? "True" : "False");
  //       break;
  //     default:
  //       return -1;
  //   }
  //   temp = &param->next;
  //   if (*temp == NULL) {
  //     printf("\n");
  //     break;
  //   }
  //   printf(" ");
  // }
  return 0;
}

int candy_std_type(candy_object_t param) {


  return 0;
}
