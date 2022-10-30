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
#include "candy.h"
#include "src/candy_state.h"

candy::candy() :
_cdy((void *)candy_state_create()) {
  static_cast<candy_state *>(_cdy)->ud = this;
}

candy::~candy() {
  candy_state_delete((candy_state **)&_cdy);
}

int candy::cfunc_wrap(void *cdy) {
  // candy *c = (candy *)static_cast<candy_state *>(cdy)->ud;
  return 0;
}

int candy::regist(const char name[], std::function<void(candy *)> &func) {
  printf("regist '%s'\n", name);
  return 0;
}

int candy::execute(const char func[], int nargs, int nresults) {
  printf("into %s, func '%s'\n", __FUNCTION__, func);
  return 0;
}

void candy::push_integer(int val) {
  printf("into %s\n", __FUNCTION__);
}

void candy::push_float(double val) {
  printf("into %s\n", __FUNCTION__);
}

void candy::push_boolean(bool val) {
  printf("into %s\n", __FUNCTION__);
}

void candy::push_string(std::string val) {
  printf("into %s\n", __FUNCTION__);
}

int candy::pull_integer() {
  printf("into %s\n", __FUNCTION__);
  return 0;
}

double candy::pull_float() {
  printf("into %s\n", __FUNCTION__);
  return 0;
}

bool candy::pull_boolean() {
  printf("into %s\n", __FUNCTION__);
  return 0;
}

std::string candy::pull_string() {
  printf("into %s\n", __FUNCTION__);
  return "";
}
