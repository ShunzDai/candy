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

#ifdef CANDY_TYPE_ENUM
#undef CANDY_TYPE_ENUM
#define CANDY_TYPE(_type, _name, ...) CANDY_TYPE_##_name,
#endif /* CANDY_TYPE_ENUM */

#ifdef CANDY_TYPE_STR
#undef CANDY_TYPE_STR
#define CANDY_TYPE(_type, _name, ...) #_name,
#endif /* CANDY_TYPE_STR */

#ifdef CANDY_TYPE_SIZE
#undef CANDY_TYPE_SIZE
#define CANDY_TYPE(_type, _name, ...) sizeof(_type),
#endif /* CANDY_TYPE_SIZE */
