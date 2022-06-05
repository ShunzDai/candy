/**
  * Copyright 2022 ShunzDai
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
#include "candy_vm.h"
#include "src/struct/candy_object.h"
#include "src/core/candy_parser.h"

int candy_vm_runcode(candy_object_t root, char * const code){
  candy_assert(root != NULL);
  candy_assert(code != NULL);
  candy_lexer_t lex = candy_lexer_create(code);
  candy_wrap_t wrap = NULL;
  candy_lexer_get_token(lex, &wrap);
  if (wrap)
    candy_wrap_delete(wrap);
  lex = candy_lexer_delete(lex);
  return 0;
}
