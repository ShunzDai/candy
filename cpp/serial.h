/**
  * Copyright 2022-2023 ShunzDai
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

#include <string.h>
#include <assert.h>
#include <utility>
#include <string_view>
#include <string>

class serial {
  public:
  using ibuf_t = std::basic_string_view<uint8_t>;
  using obuf_t = std::basic_string<uint8_t>;

  template <typename ... arg_t>
  constexpr static obuf_t pack(const arg_t & ... args);
  template <typename ... arg_t>
  constexpr static std::tuple<arg_t ...> unpack(ibuf_t &&ibuf);

  private:
  template<typename first_t, typename ... rest_t>
  struct is_tuple {
    static const bool value =
    std::is_same<first_t, std::tuple<>>::value;
  };

  template<typename first_t, typename ... rest_t>
  struct is_tuple<std::tuple<first_t, rest_t ...>> {
    static const bool value = true;
  };

  template<typename first_t, typename ... rest_t>
  struct is_pair {
    static const bool value = false;
  };

  template<typename first_t, typename ... rest_t>
  struct is_pair<std::pair<first_t, rest_t ...>> {
    static const bool value = true;
  };

  template <typename arg_t>
  struct is_base {
    static const bool value =
    std::is_integral<arg_t>::value ||
    std::is_floating_point<arg_t>::value;
  };

  template <typename arg_t>
  struct is_string {
    static const bool value =
    std::is_same<arg_t,      std::string>::value ||
    std::is_same<arg_t, std::string_view>::value;
  };

  template <typename arg_t>
  constexpr static obuf_t pack_one(const arg_t &arg);
  template <typename ... arg_t, size_t ... seq>
  constexpr static obuf_t pack_one(const std::tuple<arg_t ...> &arg, std::index_sequence<seq ...>);
  template <typename arg_t>
  constexpr static arg_t unpack_one(ibuf_t &ibuf);
};

template <typename arg_t>
constexpr serial::obuf_t serial::pack_one(const arg_t &arg) {
  if constexpr (std::is_same<arg_t, const char *>::value) {
    return obuf_t((const uint8_t *)arg, strlen(arg) + 1);
  }
  else if constexpr (is_string<arg_t>::value) {
    return obuf_t((const uint8_t *)arg.data(), arg.size()) + obuf_t((const uint8_t *)"\0", 1);
  }
  else if constexpr (is_tuple<arg_t>::value) {
    return pack_one(std::forward<decltype(arg)>(arg), std::make_index_sequence<std::tuple_size<arg_t>::value> {});
  }
  else if constexpr (is_pair<arg_t>::value) {
    return pack_one(std::forward<decltype(arg.first)>(arg.first)) + pack_one(std::forward<decltype(arg.second)>(arg.second));
  }
  else if constexpr (is_base<arg_t>::value) {
    return obuf_t((const uint8_t *)&arg, sizeof(arg_t));
  }
  else {
    static_assert(!std::is_same<arg_t, arg_t>::value, "unknown arg type");
  }
}

template <typename ... arg_t, size_t ... seq>
constexpr serial::obuf_t serial::pack_one(const std::tuple<arg_t ...> &arg, std::index_sequence<seq ...>) {
  return (pack_one(std::get<seq>(std::forward<decltype(arg)>(arg))) + ...);
}

template <typename ... arg_t>
constexpr serial::obuf_t serial::pack(const arg_t & ... args) {
  if constexpr (sizeof...(args) == 0)
    return {};
  return (pack_one(std::forward<decltype(args)>(args)) + ...);
}

template <typename arg_t>
constexpr arg_t serial::unpack_one(ibuf_t &ibuf) {
  if constexpr (std::is_same<arg_t, char *>::value) {
    arg_t arg = (char *)ibuf.data();
    ibuf.remove_prefix(strlen((const char *)ibuf.data()) + 1);
    return arg;
  }
  else if constexpr (is_base<arg_t>::value) {
    arg_t &arg = *(arg_t *)ibuf.data();
    ibuf.remove_prefix(sizeof(arg_t));
    return arg;
  }
  else {
    static_assert(!std::is_same<arg_t, arg_t>::value, "unknown arg type");
  }
}

template <typename ... arg_t>
constexpr std::tuple<arg_t ...> serial::unpack(ibuf_t &&ibuf) {
  assert(ibuf.data() && ibuf.size());
  return {unpack_one<arg_t>(ibuf) ...};
}
