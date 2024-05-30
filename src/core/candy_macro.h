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
#ifndef CANDY_CORE_MACRO_H
#define CANDY_CORE_MACRO_H
#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#define CANDY_VA_CASE0(_rule, _glue)
#define CANDY_VA_CASE1(_rule, _glue, _arg0)                                                                 _rule(_arg0)
#define CANDY_VA_CASE2(_rule, _glue, _arg0, _arg1)        _glue(CANDY_VA_CASE1(_rule, _glue, _arg0),        _rule(_arg1))
#define CANDY_VA_CASE3(_rule, _glue, _arg0, _arg1, _arg2) _glue(CANDY_VA_CASE2(_rule, _glue, _arg0, _arg1), _rule(_arg2))
#define CANDY_VA_SELECT(_0, _1, _2, _3, _n, ...)          CANDY_VA_CASE##_n
#define CANDY_VA(_rule, _glue, ...)                       CANDY_VA_SELECT(_, ##__VA_ARGS__, 3, 2, 1, 0)(_rule, _glue, ##__VA_ARGS__)

#ifdef __cplusplus
}
#endif /* __cplusplus */
#endif /* CANDY_CORE_MACRO_H */
