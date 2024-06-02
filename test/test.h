/**
  * Copyright 2022-2024 ShunzDai
  *
  * Licensed under the Apache License, Version 2.0 (the "License");
  * you may not use this file except in compliance with the License.
  * You may obtain a copy of the License at
  *
  *     http://www.apache.org/licenses/LICENSE-2.0
  *
  * Unless required by applicable law or agreed to in writing, software
  * distributed under the License is distributed on an "AS IS" BASIS,
  * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  * See the License for the specific language governing permissions and
  * limitations under the License.
  */
#include "gtest/gtest.h"
#include <stdlib.h>

#define unique_name_body(test_suite_name, counter) test_suite_name##_##counter
#define unique_name_impl(test_suite_name, counter) unique_name_body(test_suite_name, counter)
#define unique_name(test_suite_name) unique_name_impl(test_suite_name, __COUNTER__)

#define EXPECT_MEMEQ(m1, m2, n) EXPECT_EQ(memcmp(m1, m2, n), 0)

static inline void *test_allocator(void *ptr, size_t old_size, size_t new_size, void *arg) {
  if (new_size)
    return realloc(ptr, new_size);
  free(ptr);
  return NULL;
}
