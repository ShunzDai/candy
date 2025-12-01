/**
  * Copyright 2022-2025 ShunzDai
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

#include "candy.h"
#include "core/candy_state.h"
#include <tuple>
#include <type_traits>
#include <initializer_list>

class candy {
public:
  candy();

  template <typename res_t, typename ... arg_t>
  candy_err_t regist(const char name[], res_t (*fn)(arg_t ...)) {
    candy_err_t err = CANDY_OK;
    err = candy_state_push_cfunc(self, +[](candy_state_t *self) {
      return 0;
    });
    return err;
  }

  template <typename ... arg_t>
  candy_err_t call(const char name[], const arg_t & ... arg) {
    candy_err_t err = CANDY_OK;
    return err;
  }

private:
  candy_state_t *self;

  candy_err_t push() {
    candy_err_t err = CANDY_OK;
    return err;
  }

  template <typename first_t, typename ... arg_t>
  candy_err_t push(const first_t &val, const arg_t & ... arg) {
    candy_err_t err = CANDY_OK;
    if constexpr (std::is_integral_v<first_t>) {
      err = candy_state_push_integer(self, val);
    }
    else if constexpr (std::is_floating_point_v<first_t>) {
      err = candy_state_push_float(self, val);
    }
    else {
      static_assert(!std::is_same_v<first_t, first_t>);
    }
    err = push(arg ...);
    return err;
  }

  template <typename res_t>
  res_t to(int idx) {
    if constexpr (std::is_integral_v<res_t>) {
      return candy_state_to_integer(self, idx);
    }
    else if constexpr (std::is_floating_point_v<res_t>) {
      return candy_state_to_float(self, idx);
    }
    else {
      static_assert(!std::is_same_v<res_t, res_t>);
    }
  }

  template <typename ... res_t, size_t ... seq>
  std::tuple<res_t ...> to_tuple(std::index_sequence<seq ...>) {
    return {to<res_t>(seq) ...};
  }

  template <typename ... res_t>
  std::tuple<res_t ...> to_tuple() {
    return to_tuple(std::make_index_sequence<sizeof...(res_t)> {});
  }
};
