/* Standalone compile/execution split; no production API or CMake changes.
 * Build via profile_calls.py --core ... --binary build/profile_calls --build-only.
 * Direct invocation: profile_calls CALLS REPS WARMUP [unrolled|loop]
 * Timed execution retains normal VM GC polling, but excludes explicit full GC,
 * source generation, compilation, warmup, validation, and state teardown.
 */
#define _POSIX_C_SOURCE 200809L
#include "core/candy.h"
#include "core/candy_array.h"
#include "core/candy_closure.h"
#include "core/candy_exception.h"
#include "core/candy_gc.h"
#include "core/candy_object.h"
#include "core/candy_parser.h"
#include "core/candy_proto.h"
#include "core/candy_reader.h"
#include "core/candy_state.h"
#include "core/candy_table.h"
#include <errno.h>
#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

static candy_gc_t *collector;

/* Same dispatch as candy.c; capture only during this single-state process.
 * Do not duplicate the private candy_state layout or expose a new VM accessor.
 */
static candy_err_t handler(candy_object_t *object, candy_gc_t *gc,
                           candy_events_t event, void *arg) {
  collector = gc;
  if (candy_object_mask(object) & MASK_ARRAY)
    return candy_array_handler((candy_array_t *)object, gc, event, arg);
  switch (candy_object_type(object)) {
    case CANDY_TYPE_CCLOS: return candy_cclosure_handler((candy_cclosure_t *)object, gc, event, arg);
    case CANDY_TYPE_SCLOS: return candy_sclosure_handler((candy_sclosure_t *)object, gc, event, arg);
    case CANDY_TYPE_TABLE: return candy_table_handler((candy_table_t *)object, gc, event, arg);
    case CANDY_TYPE_PROTO: return candy_proto_handler((candy_proto_t *)object, gc, event, arg);
    case CANDY_TYPE_STATE: return candy_state_handler((candy_state_t *)object, gc, event, arg);
    default: return CANDY_ERR;
  }
}

static void *allocator(void *previous, size_t old_size, size_t new_size, void *arg) {
  (void)old_size;
  (void)arg;
  if (new_size) return realloc(previous, new_size);
  free(previous);
  return NULL;
}

static uint64_t now_ns(void) {
  struct timespec t;
  if (clock_gettime(CLOCK_MONOTONIC, &t) != 0) {
    perror("clock_gettime");
    exit(1);
  }
  return (uint64_t)t.tv_sec * UINT64_C(1000000000) + (uint64_t)t.tv_nsec;
}

static int number(const char *text, size_t *out, int allow_zero) {
  char *end;
  errno = 0;
  if (*text == '-' || !*text) return 0;
  unsigned long long n = strtoull(text, &end, 10);
  if (errno || *end || n > SIZE_MAX || (!allow_zero && !n)) return 0;
  *out = (size_t)n;
  return 1;
}

static char *source_text(size_t calls, int loop, size_t *size) {
  const char head[] = "def id(n)\nreturn n\nend\n";
  const char statement[] = "id(1)\n";
  const char tail[] = "return id(1)\n";
  size_t capacity = sizeof(head) + sizeof(tail) + 256;
  if (!loop) {
    if (calls > (SIZE_MAX - capacity) / (sizeof(statement) - 1)) return NULL;
    capacity += calls * (sizeof(statement) - 1);
  }
  char *source = malloc(capacity);
  if (!source) return NULL;
  size_t used = sizeof(head) - 1;
  memcpy(source, head, used);
  if (loop) {
    int count = snprintf(source + used, capacity - used,
      "i = 0\nwhile (i < %zu)\nid(1)\ni = i + 1\nend\n", calls);
    if (count < 0 || (size_t)count >= capacity - used) { free(source); return NULL; }
    used += (size_t)count;
  } else {
    for (size_t i = 0; i < calls; ++i) {
      memcpy(source + used, statement, sizeof(statement) - 1);
      used += sizeof(statement) - 1;
    }
  }
  memcpy(source + used, tail, sizeof(tail));
  *size = used + sizeof(tail) - 1;
  return source;
}

static int invoke(candy_state_t *state, candy_object_t *closure, int validate) {
  if (candy_state_push_object(state, closure) != CANDY_OK ||
      candy_state_call(state, 0, validate ? 1 : 0) != CANDY_OK) {
    fprintf(stderr, "precompiled call failed\n");
    return 0;
  }
  if (validate && (candy_state_get_type(state, -1) != CANDY_TYPE_INTEGER ||
                   candy_state_to_integer(state, -1) != 1)) {
    fprintf(stderr, "precompiled result is not integer 1\n");
    return 0;
  }
  return 1;
}

int main(int argc, char **argv) {
  size_t calls, reps, warmup;
  if ((argc != 4 && argc != 5) || !number(argv[1], &calls, 1) ||
      !number(argv[2], &reps, 0) || !number(argv[3], &warmup, 1) ||
      (argc == 5 && strcmp(argv[4], "unrolled") && strcmp(argv[4], "loop"))) {
    fprintf(stderr, "usage: %s CALLS REPS WARMUP [unrolled|loop]\n", argv[0]);
    return 2;
  }
  int loop = argc == 5 && !strcmp(argv[4], "loop");
  size_t source_size;
  char *source = source_text(calls, loop, &source_size);
  uint64_t *times = reps <= SIZE_MAX / sizeof(*times) ? calloc(reps, sizeof(*times)) : NULL;
  if (!source || !times) { fprintf(stderr, "allocation failed\n"); free(source); free(times); return 1; }
  candy_state_t *state = candy_state_create(handler, allocator, NULL);
  if (!state) { fprintf(stderr, "state creation failed\n"); free(source); free(times); return 1; }
  int status = 1;
  /* dostream's explicit collection dispatches the actual state's GC handler.
   * A fresh string may not dispatch HASH/COMPARE, so setglobal alone cannot
   * reliably capture it. This tiny bootstrap is outside all measured phases.
   */
  if (candy_dostring(state, "return 1\n", 9) != CANDY_OK || !collector) {
    fprintf(stderr, "collector bootstrap failed\n");
    goto cleanup;
  }
  candy_excep_t ctx;
  candy_excep_init(&ctx);
  candy_object_t *closure = NULL;
  struct str_info reader = {source, source_size, 0};
  uint64_t begin = now_ns();
  candy_err_t error = candy_parse(collector, &ctx, string_reader, &reader, &closure);
  uint64_t compile_ns = now_ns() - begin;
  candy_excep_deinit(&ctx);
  if (error != CANDY_OK || !closure) { fprintf(stderr, "compilation failed (%d)\n", error); goto cleanup; }
  /* Permanent stack root above bootstrap result; nres=0 consumes each copy.
   * Two validations plus bootstrap leave three bounded results (no pop API).
   */
  if (candy_state_push_object(state, closure) != CANDY_OK) goto cleanup;
  begin = now_ns();
  if (candy_gc_full(collector) != CANDY_OK) goto cleanup;
  uint64_t compile_gc_ns = now_ns() - begin;
  if (!invoke(state, closure, 1)) goto cleanup;
  for (size_t i = 0; i < warmup; ++i)
    if (!invoke(state, closure, 0)) goto cleanup;
  begin = now_ns();
  if (candy_gc_full(collector) != CANDY_OK) goto cleanup;
  uint64_t warmup_gc_ns = now_ns() - begin;
  uint64_t total = 0;
  for (size_t i = 0; i < reps; ++i) {
    begin = now_ns();
    if (!invoke(state, closure, 0)) goto cleanup;
    times[i] = now_ns() - begin;
    total += times[i];
  }
  if (!invoke(state, closure, 1)) goto cleanup;
  begin = now_ns();
  if (candy_gc_full(collector) != CANDY_OK) goto cleanup;
  uint64_t execute_gc_ns = now_ns() - begin;
  printf("{\"schema\":1,\"engine\":\"candy\",\"clock\":\"monotonic_wall\","
         "\"unit\":\"ns\",\"mode\":\"%s\",\"calls\":%zu,\"calls_per_execution\":%zu,"
         "\"source_bytes\":%zu,\"reps\":%zu,\"warmup\":%zu,\"compile_ns\":%" PRIu64 ","
         "\"compile_gc_ns\":%" PRIu64 ",\"warmup_gc_ns\":%" PRIu64 ","
         "\"execute_gc_ns\":%" PRIu64 ",\"execute_total_ns\":%" PRIu64 ","
         "\"result\":1,\"runtime_gc\":\"normal\",\"execute_ns\":[",
         loop ? "loop" : "unrolled", calls, calls + 1, source_size, reps, warmup,
         compile_ns, compile_gc_ns, warmup_gc_ns, execute_gc_ns, total);
  for (size_t i = 0; i < reps; ++i) printf("%s%" PRIu64, i ? "," : "", times[i]);
  puts("]}");
  status = 0;
cleanup:
  if (candy_state_close(state) != CANDY_OK) status = 1;
  free(source);
  free(times);
  return status;
}
