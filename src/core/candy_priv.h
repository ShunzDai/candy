/**
  * Copyright 2022-2026 ShunzDai
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
#include <inttypes.h>
#include <assert.h>
#include <stdio.h> /* todo: remove */

#ifdef __GNUC__
#define CANDY_NORETURN                    __attribute__((noreturn))
#define CANDY_HOT                         __attribute__((hot))
#define CANDY_NOINLINE                    __attribute__((noinline))
#define CANDY_LIKELY(_x)                  __builtin_expect(!!(_x), 1)
#define CANDY_UNLIKELY(_x)                __builtin_expect(!!(_x), 0)
#define CANDY_FORMAT(fmtarg, firstvararg) __attribute__((format(printf, fmtarg, firstvararg)))
#define CANDY_FORCE_INLINE                inline __attribute__((always_inline))
#define CANDY_ALWAYS_INLINE               __attribute__((always_inline))
#if defined(__clang__)
#define CANDY_LTO_INLINE_DECL             CANDY_ALWAYS_INLINE
#define CANDY_LARGE_LTO_INLINE            CANDY_ALWAYS_INLINE
#else
/* GCC cannot enforce always_inline from a declaration without the body.
 * Keep large helpers out of the VM dispatch loop; small helper definitions
 * are still forced inline during LTO. */
#define CANDY_LTO_INLINE_DECL
#define CANDY_LARGE_LTO_INLINE
#endif
#else
#define CANDY_NORETURN
#define CANDY_FORMAT(fmtarg, firstvararg)
#define CANDY_FORCE_INLINE                inline
#define CANDY_ALWAYS_INLINE
#define CANDY_LTO_INLINE_DECL
#define CANDY_LARGE_LTO_INLINE
#endif

#define candy_assert(_self, _gc, _condition, _err, _format, ...) \
((_condition) ? ((void)0U) : candy_excep_throw(_self, _err, \
  (candy_object_t *)candy_array_print(_gc, _self, "%s error: " _format, candy_err_str(_err), ##__VA_ARGS__) \
))

typedef enum candy_masks {
  MASK_NONE     = (uint8_t)(0),
  MASK_OBJECT   = (uint8_t)(1 << 0),
  MASK_CONST    = (uint8_t)(1 << 1),
  MASK_ARRAY    = (uint8_t)(1 << 2),
  MASK_ROOT     = (uint8_t)(1 << 3),
  MASK_HASHABLE = (uint8_t)(MASK_CONST),
  /** @brief tombstone marker: a NONE-typed slot carrying MASK_OBJECT. A live
    key that carries MASK_OBJECT is always an object with a real type, so this
    type/mask pair never describes a real key; an unused slot has mask NONE. */
  MASK_TOMB     = (uint8_t)(MASK_NONE | MASK_OBJECT),
} candy_masks_t;

typedef enum candy_events {
  EVT_DELETE,
  EVT_COLOR,
  EVT_DIFFUSE,
  EVT_COMPARE,
  EVT_FORMAT,
} candy_events_t;

typedef int32_t candy_hash_t;

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
/* captured variable shared between a frame and its closures */
typedef struct candy_upvalue candy_upvalue_t;

typedef struct candy_excep candy_excep_t;

typedef union candy_inst candy_inst_t;

/* Output context for EVT_FORMAT. `quoted` asks strings to be emitted as JSON
 * string literals (quoted and escaped) instead of raw text. */
typedef struct candy_format {
  FILE *out;
  bool quoted;
  size_t level;
} candy_format_t;

typedef candy_err_t (*candy_handler_t)(candy_object_t *self, candy_gc_t *gc, candy_events_t evt, void *arg);

static inline const char *candy_event_str(candy_events_t val) {
  switch (val) {
    case EVT_DELETE:  return "deleting";
    case EVT_COLOR:   return "coloring";
    case EVT_DIFFUSE: return "diffusing";
    case EVT_COMPARE: return "comparing";
    case EVT_FORMAT:  return "formatting";
    default:          return "unknown";
  }
}

#ifdef __cplusplus
}
#endif /* __cplusplus */
#endif /* CANDY_CORE_PRIV_H */
