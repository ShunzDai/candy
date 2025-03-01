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
#ifndef CANDY_CORE_PRIV_H
#define CANDY_CORE_PRIV_H
#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#include "core/candy_log.h"
#include "core/candy_types.h"
#include <assert.h>
#include <stdio.h> /* todo: remove */

#ifdef __GNUC__
#define CANDY_NORETURN                    __attribute__((noreturn))
#define CANDY_FORMAT(fmtarg, firstvararg) __attribute__((format(printf, fmtarg, firstvararg)))
#else
#define CANDY_NORETURN
#define CANDY_FORMAT(fmtarg, firstvararg)
#endif

#define candy_assert(_self, _gc, _condition, _err, _format, ...) \
((_condition) ? ((void)0U) : candy_excep_throw(_self, _err, \
  (candy_object_t *)candy_array_print(_gc, _self, "%s error: " _format, candy_err_str(_err), ##__VA_ARGS__) \
))

typedef enum candy_masks {
  MASK_NONE   = 0,
  MASK_TOMB   = 1 << 0,
  MASK_ARRAY  = 1 << 1,
  MASK_STATIC = 1 << 2,
} candy_masks_t;

typedef enum candy_events {
  EVT_DELETE,
  EVT_COLOR,
  EVT_DIFFUSE,
  EVT_HASH,
  EVT_FORMAT,
} candy_events_t;

typedef uint32_t candy_hash_t;

typedef struct candy_memory candy_memory_t;
typedef struct candy_gc candy_gc_t;
typedef struct candy_wrap candy_wrap_t;
typedef struct candy_object candy_object_t;
typedef struct candy_vector candy_vector_t;
typedef struct candy_array candy_array_t;
typedef struct candy_table candy_table_t;
typedef struct candy_proto candy_proto_t;
typedef struct candy_userdef candy_userdef_t;
/* c-closure */
typedef struct candy_cclosure candy_cclosure_t;
/* script-closure */
typedef struct candy_sclosure candy_sclosure_t;

typedef struct candy_excep candy_excep_t;

typedef candy_err_t (*candy_handler_t)(candy_object_t *self, candy_gc_t *gc, candy_events_t evt, void *arg);

static inline const char *candy_event_str(candy_events_t val) {
  switch (val) {
    case EVT_DELETE:  return "deleting";
    case EVT_COLOR:   return "coloring";
    case EVT_DIFFUSE: return "diffusing";
    default:          return "unknown";
  }
}

static inline size_t candy_type_size(candy_types_t val) {
  return (size_t[]) {
    #define CANDY_TYPE_SIZE
    #include "core/candy_type.list"
  }[val];
}

#ifdef __cplusplus
}
#endif /* __cplusplus */
#endif /* CANDY_CORE_PRIV_H */
