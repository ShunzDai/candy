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
#ifndef CANDY_OPERATOR_LIST
#error "can only be include by candy_operator.list"
#endif /* CANDY_OPERATOR_LIST */

#ifdef CANDY_OPR
#undef CANDY_OPR
#endif /* CANDY_OPR */

#ifdef CANDY_OPR_ENUM
#undef CANDY_OPR_ENUM
#define CANDY_OPR(_name, ...) TK_##_name = gen_operator(__VA_ARGS__),
#endif /* CANDY_OPR_ENUM */

#ifdef CANDY_OPR_STR
#undef CANDY_OPR_STR
#define CANDY_OPR(_name, ...) case TK_##_name: return #_name;
#endif /* CANDY_OPR_STR */

#ifdef CANDY_OPR_TEST
#undef CANDY_OPR_TEST
#define CANDY_OPR(_name, ...) TEST_NORMAL(_name, TK_##_name, std::string{__VA_ARGS__}.data());
#endif /* CANDY_OPR_TEST */
