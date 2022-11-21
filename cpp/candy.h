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
#pragma once

#include <string>
#include <tuple>
#include <functional>

class candy {
  public:
  struct reg_t {
    const char *name;
    void *ud;
  };

  candy();
  ~candy();

  /**
    * @brief  cpp call candy function
    *         push args from c as input to the candy-function,
    *         and pull the return value of the candy-function into the c
    * @param  func candy-function
    * @param  args args of candy-function
    * @return candy-function's results
    */
  template <typename ... res_t, typename ... arg_t>
  std::tuple<res_t ...> call(const char func[], arg_t ... args);

  template <typename ... func_t>
  candy *regist(const char obj[], const std::pair<const char *, func_t> ... list);

  private:
  template<typename first_t, typename ... rest_t>
  struct is_tuple {
    static const bool value = false;
  };

  template<typename first_t, typename ... rest_t>
  struct is_tuple<std::tuple<first_t, rest_t ...>> {
    static const bool value = true;
  };

  void *_cdy;

  std::vector<std::function<int(candy *)>> _funcs;

  static int cfunc_wrap(void *cdy, void *cfunc);

  void regist(const char obj[], reg_t list[], unsigned size);

  int execute(const char func[], int nargs, int nresults);

  void push_integer(int val);
  void push_float(double val);
  void push_boolean(bool val);
  void push_string(std::string val);

  int pull_integer();
  double pull_float();
  bool pull_boolean();
  std::string pull_string();

  template <typename arg_t>
  void push(arg_t arg);
  template <typename ... arg_t>
  void push(arg_t ... args);
  template <typename ... arg_t, size_t ... seq>
  void push(std::tuple<arg_t ...> args, std::index_sequence<seq ...>);

  template <typename arg_t>
  arg_t pull();
  template <typename ... arg_t>
  std::tuple<arg_t ...> pull_tuple();

  /**
    * @brief  candy call cpp function
    *         pull args from the stack as input to the c-function,
    *         and push the return value of the c-function into the stack
    * @param  func c-function
    * @return number of c-function's results
    */
  template <typename res_t, typename ... arg_t>
  int call(res_t(*func)(arg_t ...));

  template <typename func_t>
  reg_t regist(std::pair<const char *, func_t> func);
};

template <typename ... res_t, typename ... arg_t>
std::tuple<res_t ...> candy::call(const char func[], arg_t ... args) {
  push(args ...);
  execute(func, sizeof...(arg_t), sizeof...(res_t));
  return pull_tuple<res_t ...>();
}

template <typename arg_t>
void candy::push(arg_t arg) {
  if constexpr (
    std::is_same<arg_t,   int8_t>::value ||
    std::is_same<arg_t,  int16_t>::value ||
    std::is_same<arg_t,  int32_t>::value ||
    std::is_same<arg_t,  int64_t>::value ||
    std::is_same<arg_t,  uint8_t>::value ||
    std::is_same<arg_t, uint16_t>::value ||
    std::is_same<arg_t, uint32_t>::value ||
    std::is_same<arg_t, uint64_t>::value
  )
    push_integer(arg);
  else if constexpr (
       std::is_same<arg_t,  float>::value
    || std::is_same<arg_t, double>::value
  )
    push_float(arg);
  else if constexpr (std::is_same<arg_t, bool>::value)
    push_boolean(arg);
  else if constexpr (
    std::is_same<arg_t,     const char *>::value ||
    std::is_same<arg_t,      std::string>::value ||
    std::is_same<arg_t, std::string_view>::value
  )
    push_string(arg);
  else
    static_assert(!std::is_same<arg_t, arg_t>::value, "unknown arg type");
}

template <typename ... func_t>
candy *candy::regist(const char obj[], const std::pair<const char *, func_t> ... list) {
  std::vector<reg_t> clist;
  (clist.push_back(regist(list)), ...);
  regist(obj, clist.data(), clist.size());
  return this;
}

template <typename ... arg_t>
void candy::push(arg_t ... args) {
  (push(args), ...);
}

template <typename ... arg_t, size_t ... seq>
void candy::push(std::tuple<arg_t ...> args, std::index_sequence<seq ...>) {
  (push(std::get<seq>(args)), ...);
}

template <typename arg_t>
arg_t candy::pull() {
  if constexpr (
    std::is_same<arg_t,   int8_t>::value ||
    std::is_same<arg_t,  int16_t>::value ||
    std::is_same<arg_t,  int32_t>::value ||
    std::is_same<arg_t,  int64_t>::value ||
    std::is_same<arg_t,  uint8_t>::value ||
    std::is_same<arg_t, uint16_t>::value ||
    std::is_same<arg_t, uint32_t>::value ||
    std::is_same<arg_t, uint64_t>::value
  )
    return (arg_t)pull_integer();
  else if constexpr (
       std::is_same<arg_t,  float>::value
    || std::is_same<arg_t, double>::value
  )
    return (arg_t)pull_float();
  else if constexpr (std::is_same<arg_t, bool>::value)
    return (arg_t)pull_boolean();
  else if constexpr (
    std::is_same<arg_t,      std::string>::value ||
    std::is_same<arg_t, std::string_view>::value
  )
    return (arg_t)pull_string();
  else
    static_assert(!std::is_same<arg_t, arg_t>::value, "unknown arg type");
}

template <typename ... arg_t>
std::tuple<arg_t ...> candy::pull_tuple() {
  return std::make_tuple(pull<arg_t>() ...);
}

template <typename res_t, typename ... arg_t>
int candy::call(res_t(*func)(arg_t ...)) {
  if constexpr (std::is_void<res_t>::value) {
    /* void(...) */
    if constexpr (sizeof...(arg_t)) {
      std::apply(func, pull_tuple<arg_t ...>());
      return 0;
    }
    /* void() */
    else {
      func();
      return 0;
    }
  }
  else {
    /* std::tuple<...>(...) */
    if constexpr (is_tuple<res_t>::value) {
      push(std::apply(func, pull_tuple<arg_t ...>()), std::make_index_sequence<std::tuple_size<res_t>::value> {});
      return std::tuple_size<res_t>::value;
    }
    /* type(...) */
    else {
      push(std::apply(func, pull_tuple<arg_t ...>()));
      return 1;
    }
  }
}

template <typename func_t>
candy::reg_t candy::regist(std::pair<const char *, func_t> func) {
  _funcs.push_back([func](candy *cdy) {
    return cdy->call(func.second);
  });
  return {func.first, (void *)(_funcs.size() - 1)};
}
