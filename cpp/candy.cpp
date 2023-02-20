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

namespace candy {

state::state() :
_csta((void *)candy_state_create(this)) {
  candy_regist_t list[] = {
    {"__entry__", (candy_builtin_t)entry},
    {"__deinit__", (candy_builtin_t)deinit},
  };
  candy_add_builtin((candy_state *)_csta, "__global__", list, 2);
}

state::~state() {
  candy_state_delete((candy_state **)&_csta);
}

int state::entry(void *csta) {
  state *sta = (state *)candy_ud((candy_state *)csta);
  return (*(builtin_t *)candy_callinfo((candy_state *)csta)->func)(sta);
}

int state::deinit(void *cobj) {

  return 0;
}

int state::dostring(const char exp[]) {
  return candy_dostring((candy_state *)_csta, exp);
}

int state::dofile(const char path[]) {
  return candy_dofile((candy_state *)_csta, path);
}

int state::add(const char name[], pair_t list[], size_t size) {
  return candy_add_builtin((candy_state *)_csta, name, (candy_regist_t *)list, size);
}

int state::ccall(const char name[], int nargs, int nresults) {
  return candy_call((candy_state *)_csta, name, nargs, nresults);
}

void state::push_integer(const int64_t val) {
  // printf("into %s\n", __FUNCTION__);
  candy_push_integer((candy_state *)_csta, val);
}

void state::push_float(const double val) {
  // printf("into %s\n", __FUNCTION__);
  candy_push_float((candy_state *)_csta, val);
}

void state::push_boolean(const bool val) {
  // printf("into %s\n", __FUNCTION__);
  candy_push_boolean((candy_state *)_csta, val);
}

void state::push_string(const std::string val) {
  // printf("into %s\n", __FUNCTION__);
  candy_push_string((candy_state *)_csta, val.data(), val.size());
}

int64_t state::pull_integer() {
  // printf("into %s\n", __FUNCTION__);
  return candy_pull_integer((candy_state *)_csta);
}

double state::pull_float() {
  // printf("into %s\n", __FUNCTION__);
  return candy_pull_float((candy_state *)_csta);
}

bool state::pull_boolean() {
  // printf("into %s\n", __FUNCTION__);
  return candy_pull_boolean((candy_state *)_csta);
}

std::string state::pull_string() {
  // printf("into %s\n", __FUNCTION__);
  size_t size = 0;
  return {candy_pull_string((candy_state *)_csta, &size), size};
}

}
