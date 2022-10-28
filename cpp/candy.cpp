#include "candy.h"
#include "src/candy_state.h"

candy::candy() :
_s((void *)candy_state_create()) {
  static_cast<candy_state *>(_s)->ud = this;
}

candy::~candy() {
  candy_state_delete((candy_state **)(&_s));
}

int candy::execute(const char func[], int nargs, int nresults) {
  printf("into %s\n", __FUNCTION__);
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
