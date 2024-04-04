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
#ifndef CANDY_KEYWORD_LIST
#error "can only be include by candy_keyword.list"
#endif /* CANDY_KEYWORD_LIST */

#ifdef CANDY_KW
#undef CANDY_KW
#endif /* CANDY_KW */

#ifdef CANDY_KW_ENUM
#undef CANDY_KW_ENUM
#define CANDY_KW(_keyword, ...) TK_##_keyword,
#endif /* CANDY_KW_ENUM */

#ifdef CANDY_KW_MATCH
#undef CANDY_KW_MATCH
#define CANDY_KW(_keyword, _hash) case _hash: return TK_##_keyword;
#endif /* CANDY_KW_MATCH */

#ifdef CANDY_KW_TEST
#undef CANDY_KW_TEST
#define CANDY_KW(_keyword, ...) TEST_NORMAL(_keyword, TK_##_keyword, #_keyword);
#endif /* CANDY_KW_TEST */
