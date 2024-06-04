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
#ifndef CANDY_TYPE_LIST
#error "can only be include by candy_type.list"
#endif /* CANDY_TYPE_LIST */

#ifdef CANDY_TYPE
#undef CANDY_TYPE
#endif /* CANDY_TYPE */

#define gen_extended_rule(_extd, _count, _base) CANDY_EXTD_##_extd = (_count << 4) | CANDY_BASE_##_base,
#define gen_extended_glue(_extd0, _extd1) _extd0 _extd1
#define gen_extended(_base, ...) CANDY_VA(gen_extended_rule, gen_extended_glue, ##__VA_ARGS__, _base)

#ifdef CANDY_TYPE_ENUM
#undef CANDY_TYPE_ENUM
#define CANDY_TYPE(_type, _base, ...) CANDY_BASE_##_base = __COUNTER__, gen_extended(_base, ##__VA_ARGS__)
#endif /* CANDY_TYPE_ENUM */

#ifdef CANDY_TYPE_STR
#undef CANDY_TYPE_STR
#define CANDY_TYPE(_type, _base, ...) #_base,
#endif /* CANDY_TYPE_STR */

#ifdef CANDY_TYPE_SIZE
#undef CANDY_TYPE_SIZE
#define CANDY_TYPE(_type, _base, ...) sizeof(_type),
#endif /* CANDY_TYPE_SIZE */
