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
#ifndef CANDY_CORE_EXCEPTION_H
#define CANDY_CORE_EXCEPTION_H
#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#include "core/candy_priv.h"

typedef enum candy_err {
  #define CANDY_ERR_ENUM
  #include "core/candy_error.list"
} candy_err_t;

typedef void (*candy_exce_cb_t)(void *arg);

struct candy_exce {
  candy_exce_t *prev;
};

int candy_exce_init(candy_exce_t *self);
int candy_exce_deinit(candy_exce_t *self);

candy_err_t candy_exce_try(candy_exce_t *self, candy_exce_cb_t cb, void *arg, candy_object_t **err);
void candy_exce_throw(candy_exce_t *self, candy_err_t code, candy_object_t *err) CANDY_NORETURN;
size_t candy_exce_depth(const candy_exce_t *self);

static inline const char *candy_err_str(candy_err_t err) {
  switch (err) {
    #define CANDY_ERR_STR
    #include "core/candy_error.list"
    default: return "unknown";
  }
}

#ifdef __cplusplus
}
#endif /* __cplusplus */
#endif /* CANDY_CORE_EXCEPTION_H */
