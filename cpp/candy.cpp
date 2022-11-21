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
_cdy((void *)candy_state_create(this)) {
  candy_regist_entry((candy_state *)_cdy, (candy_cfunc_t)cfunc_wrap);
}

candy::~candy() {
  candy_state_delete((candy_state **)&_cdy);
}

int candy::cfunc_wrap(void *cdy, void *cfunc) {
  candy *c = (candy *)candy_ud((candy_state *)cdy);
  return c->_funcs[(size_t)cfunc](c);
}

void candy::regist(const char obj[], reg_t list[], unsigned size) {
  printf(">>> regist obj '%s' in, list %p\n", obj, list);
  candy_regist_cfuncs((candy_state *)_cdy, obj, (candy_regist_t *)list, size);
  printf("<<< regist obj '%s' out, list %p\n", obj, list);
  return;
}

int candy::execute(const char func[], int nargs, int nresults) {
  printf("into %s, func '%s'\n", __FUNCTION__, func);
  return 0;
}

void candy::push_integer(int val) {
  printf("into %s\n", __FUNCTION__);
  candy_push_integer((candy_state *)_cdy, val);
}

void candy::push_float(double val) {
  printf("into %s\n", __FUNCTION__);
  candy_push_float((candy_state *)_cdy, val);
}

void candy::push_boolean(bool val) {
  printf("into %s\n", __FUNCTION__);
  candy_push_boolean((candy_state *)_cdy, val);
}

void candy::push_string(std::string val) {
  printf("into %s\n", __FUNCTION__);
  candy_push_string((candy_state *)_cdy, val.data(), val.size());
}

int candy::pull_integer() {
  printf("into %s\n", __FUNCTION__);
  return candy_pull_integer((candy_state *)_cdy);
}

double candy::pull_float() {
  printf("into %s\n", __FUNCTION__);
  return candy_pull_float((candy_state *)_cdy);;
}

bool candy::pull_boolean() {
  printf("into %s\n", __FUNCTION__);
  return candy_pull_boolean((candy_state *)_cdy);;
}

std::string candy::pull_string() {
  printf("into %s\n", __FUNCTION__);
  int size = -1;
  return {candy_pull_string((candy_state *)_cdy, &size), (size_t)size};
}
