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
#include "candy.h"
#include "src/candy_state.h"
#include "src/candy_builtin.h"

namespace candy {

state::state() :
_csta((void *)candy_state_create(this)) {
  candy_add_builtin((candy_state *)_csta, candy_builtin_list, candy_builtin_size);
}

state::~state() {
  candy_state_delete((candy_state **)&_csta);
}

state *state::self(void *csta) {
  return (state *)candy_ud((candy_state *)csta);
}

int state::dostring(const char exp[]) {
  return candy_dostring((candy_state *)_csta, exp);
}

int state::dofile(const char path[]) {
  return candy_dofile((candy_state *)_csta, path);
}

void state::dump_global(void) {
  candy_dump_global((candy_state *)_csta);
}

int state::add(const char name[], void *func, int (*wrap)(void *)) {
  const void *val[] = {func, (void *)wrap};
  candy_push_ud((candy_state *)_csta, val, 2);
  candy_set_global((candy_state *)_csta, name);
  return 0;
}

int state::ccall(const char name[], int nargs, int nresults) {
  candy_get_global((candy_state *)_csta, name);
  const void **val = candy_pull_ud((candy_state_t *)_csta, nullptr);
  candy_push_ud((candy_state *)_csta, &val[0], 1);
  candy_push_builtin((candy_state *)_csta, (candy_builtin_t *)&val[1], 1);
  return candy_call((candy_state *)_csta, nargs, nresults);
}

void state::push_integer(const candy_integer_t &val) {
  candy_push_integer((candy_state *)_csta, &val, 1);
}

void state::push_float(const candy_float_t &val) {
  candy_push_float((candy_state *)_csta, &val, 1);
}

void state::push_boolean(const candy_boolean_t &val) {
  candy_push_boolean((candy_state *)_csta, &val, 1);
}

void state::push_string(const std::string &val) {
  candy_push_string((candy_state *)_csta, val.data(), val.size());
}

candy_integer_t state::pull_integer(void) {
  return *candy_pull_integer((candy_state *)_csta, nullptr);
}

candy_float_t state::pull_float(void) {
  return *candy_pull_float((candy_state *)_csta, nullptr);
}

candy_boolean_t state::pull_boolean(void) {
  return *candy_pull_boolean((candy_state *)_csta, nullptr);
}

std::string state::pull_string(void) {
  size_t size = 0;
  return {candy_pull_string((candy_state *)_csta, &size), size};
}

const void *state::pull_ud(void) {
  return *candy_pull_ud((candy_state_t *)_csta, nullptr);
}

}
