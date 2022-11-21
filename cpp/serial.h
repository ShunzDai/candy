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

#include <string.h>
#include <assert.h>
#include <string_view>
#include <string>

class serial {
  public:
  template<typename first_t, typename ... rest_t>
  struct is_tuple {
    static const bool value = false;
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
  using ibuf_t = std::basic_string_view<uint8_t>;
  using obuf_t = std::basic_string<uint8_t>;
  template <typename ... arg_t>
  static constexpr obuf_t pack(arg_t && ... args);
  template <typename ... arg_t>
  static constexpr std::tuple<arg_t ...> unpack(ibuf_t ibuf);
  template <typename res_t, typename ... arg_t>
  int call(res_t(*func)(arg_t ...));
  private:
  template <typename arg_t>
  static constexpr void pack_one(obuf_t &obuf, arg_t && arg);
  template <typename ... arg_t, size_t ... seq>
  static constexpr void pack_one(obuf_t &obuf, std::tuple<arg_t ...> && arg, std::index_sequence<seq ...>);
  template <typename arg_t>
  static constexpr arg_t unpack_one(ibuf_t & ibuf);
};

template <typename arg_t>
constexpr void serial::pack_one(obuf_t &obuf, arg_t && arg) {
  if constexpr (std::is_same<arg_t, const char *>::value) {
    obuf.append((const uint8_t *)arg, strlen(arg) + 1);
  }
  else if constexpr (std::is_same<arg_t, std::string>::value) {
    obuf.append((const uint8_t *)arg.data(), arg.size());
    obuf.append((const uint8_t *)"\0", 1);
  }
  else if constexpr (is_tuple<arg_t>::value) {
    pack_one(obuf, std::forward<decltype(arg)>(arg), std::make_index_sequence<std::tuple_size<arg_t>::value> {});
  }
  else if constexpr (is_pair<arg_t>::value) {
    pack_one(obuf, std::forward<decltype(arg.first)>(arg.first));
    pack_one(obuf, std::forward<decltype(arg.second)>(arg.second));
  }
  else if constexpr (
    std::is_same<arg_t,   int8_t>::value ||
    std::is_same<arg_t,  int16_t>::value ||
    std::is_same<arg_t,  int32_t>::value ||
    std::is_same<arg_t,  int64_t>::value ||
    std::is_same<arg_t,  uint8_t>::value ||
    std::is_same<arg_t, uint16_t>::value ||
    std::is_same<arg_t, uint32_t>::value ||
    std::is_same<arg_t, uint64_t>::value ||
    std::is_same<arg_t,    float>::value ||
    std::is_same<arg_t,   double>::value ||
  std::is_same<arg_t,     bool>::value
  ) {
    obuf.append((const uint8_t *)&arg, sizeof(arg_t));
  }
  else {
    assert(0);
  }
}

template <typename ... arg_t, size_t ... seq>
constexpr void serial::pack_one(obuf_t &obuf, std::tuple<arg_t ...> && arg, std::index_sequence<seq ...>) {
  (pack_one(obuf, std::get<seq>(std::forward<decltype(arg)>(arg))), ...);
}

template <typename ... arg_t>
constexpr serial::obuf_t serial::pack(arg_t && ... args) {
  if constexpr (sizeof...(args) == 0)
    return {};
  obuf_t obuf;
  (pack_one(obuf, std::forward<decltype(args)>(args)), ...);
  return obuf;
}

template <typename arg_t>
constexpr arg_t serial::unpack_one(ibuf_t & ibuf) {
  arg_t arg;
  if constexpr (std::is_same<arg_t, std::string>::value) {
    arg = std::string((const char *)ibuf.data(), strlen((const char *)ibuf.data()) + 1);
    ibuf.remove_prefix(strlen((const char *)ibuf.data()) + 1);
  }
  else if constexpr (
    std::is_same<arg_t,   int8_t>::value ||
    std::is_same<arg_t,  int16_t>::value ||
    std::is_same<arg_t,  int32_t>::value ||
    std::is_same<arg_t,  int64_t>::value ||
    std::is_same<arg_t,  uint8_t>::value ||
    std::is_same<arg_t, uint16_t>::value ||
    std::is_same<arg_t, uint32_t>::value ||
    std::is_same<arg_t, uint64_t>::value ||
    std::is_same<arg_t,    float>::value ||
    std::is_same<arg_t,   double>::value ||
  std::is_same<arg_t,     bool>::value
  ) {
    arg = *(arg_t *)ibuf.data();
    ibuf.remove_prefix(sizeof(arg_t));
  }
  else {
    assert(0);
  }
  return arg;
}

template <typename ... arg_t>
constexpr std::tuple<arg_t ...> serial::unpack(ibuf_t ibuf) {
  return {unpack_one<arg_t>(ibuf) ...};
}
