#pragma once

#include <string>
#include <tuple>

class candy {
  public:
  candy();
  ~candy();

  /**
  * @brief  candy call cpp function
  *         pull args from the stack as input to the c-function,
  *         and push the return value of the c-function into the stack
  * @param  func c-function
  * @return number of c-function's results
  */
  template <typename res_t, typename... arg_t>
  int call(res_t(*func)(arg_t ...));

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

  private:
  template<typename T, typename ... Types>
  struct is_tuple {
    static const bool value = false;
  };

  template<typename T, typename ... Types>
  struct is_tuple<std::tuple<T, Types ...>> {
    static const bool value = true;
  };

  void *_s;

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
};

template <typename res_t, typename ... arg_t>
int candy::call(res_t(*func)(arg_t ...)) {
  if constexpr (std::is_void<res_t>::value) {
    /* void() */
    if constexpr (sizeof...(arg_t) == 0) {
      func();
      return 0;
    }
    /* void(...) */
    else {
      std::apply(func, std::make_tuple(pull<arg_t>() ...));
      return 0;
    }

  }
  else {
    /* std::tuple<...>(...) */
    if constexpr (is_tuple<res_t>::value) {
      push(std::apply(func, std::make_tuple(pull<arg_t>() ...)), std::make_index_sequence<std::tuple_size<res_t>::value> {});
      return std::tuple_size<res_t>::value;
    }
    /* type(...) */
    else {
      push(std::apply(func, std::make_tuple(pull<arg_t>() ...)));
      return 1;
    }
  }
}

template <typename ... res_t, typename ... arg_t>
std::tuple<res_t ...> candy::call(const char func[], arg_t ... args) {
  push(args ...);
  execute(func, sizeof...(arg_t), sizeof...(res_t));
  return std::make_tuple(pull<res_t>() ...);
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
  return {};
}
