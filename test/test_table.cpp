#include "test_common.h"
#include "src/candy_table.h"
#include "src/candy_wrap.h"

TEST(table, init) {
  candy_wrap_t wrap;
  candy_wrap_set_table(&wrap, NULL, 16);
  candy_wrap_deinit(&wrap);
}
