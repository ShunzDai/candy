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
#pragma once

#include "src/candy_types.h"
#include <string>
#include <tuple>
#include <functional>

namespace candy {

class state {
  public:
  state(int argc = 0, const char *argv[] = nullptr);
  ~state();

  int dostring(const char exp[]);

  int dofile(const char path[]);

  template <typename ... func_t>
  int add(const std::tuple<const char *, func_t> & ... list);

  /**
    * @brief  cpp call candy function
    *         push args from c as input to the candy-function,
    *         and pull the return value of the candy-function into the c
    * @param  func candy-function
    * @param  args args of candy-function
    * @return candy-function's results
    */
  template <typename ... res_t, typename ... arg_t>
  std::tuple<res_t ...> call(const char func[], const arg_t & ... args);

  /**
    * @brief  candy call cpp function
    *         pull args from the stack as input to the c-function,
    *         and push the return value of the c-function into the stack
    * @param  func c-function
    * @return number of c-function's results
    */
  template <typename res_t, typename ... arg_t>
  int call(res_t(*func)(arg_t ...));

  private:
  struct builtin_t {
    const char *name;
    int (*wrap)(void *);
    void *func;
  };

  template<typename first_t, typename ... rest_t>
  struct is_tuple {
    static const bool value = std::is_same<first_t, std::tuple<>>::value;
  };

  template<typename first_t, typename ... rest_t>
  struct is_tuple<std::tuple<first_t, rest_t ...>> {
    static const bool value = true;
  };

  template <typename arg_t>
  struct is_string {
    static const bool value =
    std::is_same<arg_t,     const char *>::value ||
    std::is_same<arg_t,      std::string>::value ||
    std::is_same<arg_t, std::string_view>::value;
  };

  void *_csta;

  static state *self(void *);

  int add(const char name[], void *func, int (*wrap)(void *));
  int ccall(const char name[], int nargs, int nresults);

  void push_integer(const candy_integer_t &val);
  void push_float(const candy_float_t &val);
  void push_boolean(const candy_boolean_t &val);
  void push_string(const std::string &val);

  candy_integer_t pull_integer(void);
  candy_float_t pull_float(void);
  candy_boolean_t pull_boolean(void);
  std::string pull_string(void);
  const void *pull_ud(void);

  template <typename arg_t>
  void push(arg_t arg);
  template <typename ... arg_t, size_t ... seq>
  void push_tuple(std::tuple<arg_t ...> args, std::index_sequence<seq ...>);

  template <typename arg_t>
  arg_t pull();
  template <typename ... arg_t>
  std::tuple<arg_t ...> pull_tuple(void);
};

template <typename ... func_t>
int state::add(const std::tuple<const char *, func_t> & ... list) {
  (add(std::get<0>(list), (void *)std::get<1>(list), +[](void *csta) { return self(csta)->call((func_t)self(csta)->pull_ud()); }), ...);
  return 0;
}

template <typename ... res_t, typename ... arg_t>
std::tuple<res_t ...> state::call(const char func[], const arg_t & ... args) {
  if constexpr (sizeof...(arg_t))
    push_tuple(std::make_tuple(args ...), std::make_index_sequence<sizeof...(arg_t)> {});
  ccall(func, sizeof...(arg_t), sizeof...(res_t));
  return pull_tuple<res_t ...>();
}

template <typename res_t, typename ... arg_t>
int state::call(res_t(*func)(arg_t ...)) {
  if constexpr (std::is_void<res_t>::value) {
    std::apply(func, pull_tuple<arg_t ...>());
    return 0;
  }
  else if constexpr (is_tuple<res_t>::value) {
    push_tuple(std::apply(func, pull_tuple<arg_t ...>()), std::make_index_sequence<std::tuple_size<res_t>::value> {});
    return std::tuple_size<res_t>::value;
  }
  else {
    push(std::apply(func, pull_tuple<arg_t ...>()));
    return 1;
  }
}

template <typename arg_t>
void state::push(arg_t arg) {
  if constexpr (std::is_same<arg_t, bool>::value)
    push_boolean(arg);
  else if constexpr (std::is_integral<arg_t>::value)
    push_integer(arg);
  else if constexpr (std::is_floating_point<arg_t>::value)
    push_float(arg);
  else if constexpr (is_string<arg_t>::value)
    push_string(arg);
  else
    static_assert(!std::is_same<arg_t, arg_t>::value, "unknown arg type");
}

template <typename ... arg_t, size_t ... seq>
void state::push_tuple(std::tuple<arg_t ...> args, std::index_sequence<seq ...>) {
  (push(std::get<sizeof...(arg_t) - seq - 1>(args)), ...);
}

template <typename arg_t>
arg_t state::pull() {
  if constexpr (std::is_same<arg_t, bool>::value)
    return (arg_t)pull_boolean();
  else if constexpr (std::is_integral<arg_t>::value)
    return (arg_t)pull_integer();
  else if constexpr (std::is_floating_point<arg_t>::value)
    return (arg_t)pull_float();
  else if constexpr (is_string<arg_t>::value)
    return (arg_t)pull_string();
  else
    static_assert(!std::is_same<arg_t, arg_t>::value, "unknown arg type");
}

template <typename ... arg_t>
std::tuple<arg_t ...> state::pull_tuple(void) {
  return {pull<arg_t>() ...};
}

} /* namespace candy */
