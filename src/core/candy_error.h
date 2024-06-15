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
#ifndef CANDY_ERROR_LIST
#error "can only be include by candy_error.list"
#endif /* CANDY_ERROR_LIST */

#ifdef CANDY_ERR
#undef CANDY_ERR
#endif /* CANDY_ERR */

#ifdef CANDY_ERR_ENUM
#undef CANDY_ERR_ENUM
#define CANDY_ERR(_err, _val, _str) EXCE_##_err = _val,
#endif /* CANDY_ERR_ENUM */

#ifdef CANDY_ERR_STR
#undef CANDY_ERR_STR
#define CANDY_ERR(_err, _val, _str) case EXCE_##_err: return _str;
#endif /* CANDY_ERR_STR */
