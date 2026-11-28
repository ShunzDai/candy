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
#include "test.h"

class gc : public testing::Test {
protected:
  candy_gc_t collector{};
  candy_excep_t context{};
  bool fail = false;
  candy_events_t failing_event = EVT_COLOR;

  static candy_err_t handler(candy_object_t *object, candy_gc_t *collector,
                             candy_events_t event, void *arg) {
    auto *fixture = static_cast<gc *>(candy_gc_memory(collector)->arg);
    if (fixture->fail && event == fixture->failing_event)
      return CANDY_ERR;
    if (candy_object_mask(object) & MASK_ARRAY)
      return candy_array_handler(reinterpret_cast<candy_array_t *>(object), collector, event, arg);
    switch (candy_object_type(object)) {
      case CANDY_TYPE_TABLE:
        return candy_table_handler(reinterpret_cast<candy_table_t *>(object), collector, event, arg);
      case CANDY_TYPE_PROTO:
        return candy_proto_handler(reinterpret_cast<candy_proto_t *>(object), collector, event, arg);
      case CANDY_TYPE_SCLOS:
        return candy_sclosure_handler(reinterpret_cast<candy_sclosure_t *>(object), collector, event, arg);
      case CANDY_TYPE_CCLOS:
        return candy_cclosure_handler(reinterpret_cast<candy_cclosure_t *>(object), collector, event, arg);
      default:
        return CANDY_ERR;
    }
  }

  void SetUp() override {
    candy_excep_init(&context);
    memset(&collector, 0xA5, sizeof(collector));
    ASSERT_EQ(candy_gc_init(&collector, handler, test_allocator, this), CANDY_OK);
    EXPECT_EQ(collector.threshold, CANDY_CONFIG_GC_THRESHOLD);
    EXPECT_EQ(collector.glob, nullptr);
    EXPECT_EQ(collector.gray, nullptr);
    EXPECT_EQ(collector.list, nullptr);
    EXPECT_EQ(collector.pool, nullptr);
  }

  void TearDown() override {
    fail = false;
    EXPECT_EQ(candy_gc_deinit(&collector), CANDY_OK);
    EXPECT_EQ(candy_memory_used(candy_gc_memory(&collector)), 0U);
    candy_excep_deinit(&context);
  }

  static candy_wrap_t key(candy_integer_t value) {
    candy_wrap_t result{};
    candy_wrap_set_integer(&result, value);
    return result;
  }

  void retain(candy_object_t *object, candy_integer_t index = 1) {
    if (collector.glob == nullptr)
      candy_table_create_global(&collector, &context);
    candy_wrap_t slot = key(index), value{};
    candy_wrap_set_object(&value, object);
    ASSERT_EQ(candy_table_set(candy_gc_global(&collector), &collector, &context, &slot, &value), CANDY_OK);
  }

  void release(candy_integer_t index = 1) {
    candy_wrap_t slot = key(index);
    ASSERT_EQ(candy_table_reset(candy_gc_global(&collector), &collector, &slot), CANDY_OK);
  }

  void link(candy_table_t *table, candy_object_t *object, candy_integer_t index = 1) {
    candy_wrap_t slot = key(index), value{};
    candy_wrap_set_object(&value, object);
    ASSERT_EQ(candy_table_set(table, &collector, &context, &slot, &value), CANDY_OK);
  }

  size_t objects() const {
    size_t count = 0;
    for (candy_object_t *object = collector.list; object; object = *candy_object_next(object))
      ++count;
    return count;
  }
};

TEST_F(gc, empty_collection) {
  EXPECT_EQ(candy_gc_full(&collector), CANDY_OK);
  EXPECT_EQ(collector.threshold, CANDY_CONFIG_GC_THRESHOLD);
  EXPECT_EQ(candy_gc_fsm(&collector), GC_FSM_BEGIN);
  EXPECT_EQ(candy_memory_used(candy_gc_memory(&collector)), 0U);
  EXPECT_EQ(candy_gc_mark(&collector, nullptr), CANDY_OK);
  EXPECT_EQ(candy_wrap_mark(&CANDY_WRAP_NULL, &collector), CANDY_OK);
}

TEST_F(gc, reclaims_unreachable_objects) {
  candy_array_create(&collector, &context, CANDY_TYPE_CHAR);
  candy_table_create(&collector, &context);
  candy_proto_create(&collector, &context);
  ASSERT_EQ(objects(), 3U);
  ASSERT_EQ(candy_gc_full(&collector), CANDY_OK);
  EXPECT_EQ(objects(), 0U);
  EXPECT_EQ(candy_memory_used(candy_gc_memory(&collector)), 0U);
}

TEST_F(gc, retains_reachable_graph_and_reclaims_removed_roots) {
  auto *table = candy_table_create(&collector, &context);
  auto *leaf = candy_array_create(&collector, &context, CANDY_TYPE_CHAR);
  link(table, reinterpret_cast<candy_object_t *>(leaf));
  retain(reinterpret_cast<candy_object_t *>(table));
  candy_proto_create(&collector, &context);
  for (int cycle = 0; cycle < 4; ++cycle) {
    ASSERT_EQ(candy_gc_full(&collector), CANDY_OK);
    EXPECT_EQ(objects(), 2U);
    EXPECT_EQ(candy_object_mark(reinterpret_cast<candy_object_t *>(leaf)), MARK_WHITE);
    EXPECT_EQ(candy_object_mark(collector.glob), MARK_WHITE);
    EXPECT_EQ(collector.gray, nullptr);
  }
  release();
  ASSERT_EQ(candy_gc_full(&collector), CANDY_OK);
  EXPECT_EQ(objects(), 0U);
}

TEST_F(gc, collects_cycles_after_root_removed) {
  auto *first = candy_table_create(&collector, &context);
  auto *second = candy_table_create(&collector, &context);
  link(first, reinterpret_cast<candy_object_t *>(second));
  link(second, reinterpret_cast<candy_object_t *>(first));
  retain(reinterpret_cast<candy_object_t *>(first));
  ASSERT_EQ(candy_gc_full(&collector), CANDY_OK);
  EXPECT_EQ(objects(), 2U);
  release();
  ASSERT_EQ(candy_gc_full(&collector), CANDY_OK);
  EXPECT_EQ(objects(), 0U);
}

TEST_F(gc, shared_references_are_marked_once) {
  auto *leaf = candy_array_create(&collector, &context, CANDY_TYPE_CHAR);
  auto *first = candy_table_create(&collector, &context);
  auto *second = candy_table_create(&collector, &context);
  link(first, reinterpret_cast<candy_object_t *>(leaf));
  link(second, reinterpret_cast<candy_object_t *>(leaf));
  retain(reinterpret_cast<candy_object_t *>(first), 1);
  retain(reinterpret_cast<candy_object_t *>(second), 2);
  ASSERT_EQ(candy_gc_full(&collector), CANDY_OK);
  EXPECT_EQ(objects(), 3U);
  release(1);
  ASSERT_EQ(candy_gc_full(&collector), CANDY_OK);
  EXPECT_EQ(objects(), 2U);
  release(2);
  ASSERT_EQ(candy_gc_full(&collector), CANDY_OK);
  EXPECT_EQ(objects(), 0U);
}

TEST_F(gc, preserves_constants_through_closure_and_prototype) {
  const char text[] = "constant";
  auto *constant = candy_array_create_const(&collector, &context, CANDY_TYPE_CHAR, text, sizeof(text) - 1);
  auto *proto = candy_proto_create(&collector, &context);
  candy_wrap_t value{};
  candy_wrap_set_object(&value, reinterpret_cast<candy_object_t *>(constant));
  candy_proto_add_const(proto, &collector, &context, &value);
  auto *closure = candy_sclosure_create(&collector, &context, proto, 0);
  retain(reinterpret_cast<candy_object_t *>(closure));
  ASSERT_EQ(candy_gc_full(&collector), CANDY_OK);
  EXPECT_EQ(objects(), 2U);
  EXPECT_EQ(candy_gc_find(&collector, CANDY_TYPE_CHAR, text, sizeof(text) - 1,
                        hash_djb(text, sizeof(text) - 1)), reinterpret_cast<candy_object_t *>(constant));
  release();
  ASSERT_EQ(candy_gc_full(&collector), CANDY_OK);
  EXPECT_EQ(objects(), 0U);
  EXPECT_EQ(candy_gc_find(&collector, CANDY_TYPE_CHAR, text, sizeof(text) - 1,
                        hash_djb(text, sizeof(text) - 1)), nullptr);
}

TEST_F(gc, constant_pool_finds_slots_for_many_unique_strings) {
  /* The probe sequence has to reach every slot: a stride that only covers part
   * of the table can miss the vacant slot and make interning fail. */
  std::vector<const candy_object_t *> objects;
  for (int index = 0; index < 2000; ++index) {
    char text[16];
    const int len = std::snprintf(text, sizeof(text), "s%d", index);
    auto *object = candy_array_create_const(&collector, &context, CANDY_TYPE_CHAR, text, (size_t)len);
    ASSERT_NE(object, nullptr) << "interning failed at " << index;
    objects.push_back(reinterpret_cast<const candy_object_t *>(object));
  }
  for (int index = 0; index < 2000; ++index) {
    char text[16];
    const int len = std::snprintf(text, sizeof(text), "s%d", index);
    auto *object = candy_array_create_const(&collector, &context, CANDY_TYPE_CHAR, text, (size_t)len);
    EXPECT_EQ(reinterpret_cast<const candy_object_t *>(object), objects[(size_t)index]);
  }
}

TEST_F(gc, constant_pool_reuses_tombstones) {
  const char text[] = "temporary";
  size_t retained_bytes = 0;
  for (int cycle = 0; cycle < 50; ++cycle) {
    auto *first = candy_array_create_const(&collector, &context, CANDY_TYPE_CHAR, text, sizeof(text) - 1);
    auto *second = candy_array_create_const(&collector, &context, CANDY_TYPE_CHAR, text, sizeof(text) - 1);
    ASSERT_EQ(first, second);
    ASSERT_EQ(candy_gc_full(&collector), CANDY_OK);
    ASSERT_EQ(candy_gc_find(&collector, CANDY_TYPE_CHAR, text, sizeof(text) - 1,
                           hash_djb(text, sizeof(text) - 1)), nullptr);
    if (cycle == 0)
      retained_bytes = candy_memory_used(candy_gc_memory(&collector));
    EXPECT_EQ(candy_memory_used(candy_gc_memory(&collector)), retained_bytes);
  }
}

TEST_F(gc, djb_hash_matches_bytewise_reference) {
  EXPECT_EQ(hash_djb(nullptr, 0), 5381);
  /* Exercise unaligned input, embedded zeros, and bytes above ASCII. */
  alignas(uint32_t) uint8_t storage[257];
  uint8_t *bytes = storage + 1;
  for (size_t i = 0; i < sizeof(storage) - 1; ++i)
    bytes[i] = static_cast<uint8_t>(137 * i + 255);

  uint64_t expected = 5381;
  for (size_t len = 0; len < sizeof(storage); ++len) {
    EXPECT_EQ(hash_djb(bytes, len), static_cast<candy_hash_t>(expected)) << "length " << len;
    if (len < sizeof(storage) - 1)
      expected = (expected * 33 + bytes[len]) & 0x7FFFFFFFu;
  }
}

TEST_F(gc, prehashed_strings_still_compare_bytes_on_collision) {
  const char first[] = "ab";
  const char second[] = "bA";
  const size_t len = sizeof(first) - 1;
  candy_hash_t hash = hash_djb(first, len);
  ASSERT_EQ(hash, hash_djb(second, len));

  auto *left = candy_array_create_const_hashed(&collector, &context, CANDY_TYPE_CHAR, first, len, hash);
  auto *right = candy_array_create_const_hashed(&collector, &context, CANDY_TYPE_CHAR, second, len, hash);
  ASSERT_NE(left, right);
  EXPECT_MEMEQ(candy_array_data(left), first, sizeof(first));
  EXPECT_MEMEQ(candy_array_data(right), second, sizeof(second));
  EXPECT_EQ(candy_array_create_const(&collector, &context, CANDY_TYPE_CHAR, first, len), left);
  EXPECT_EQ(candy_array_create_const_hashed(&collector, &context, CANDY_TYPE_CHAR, second, len, hash), right);

  auto *retained = reinterpret_cast<candy_object_t *>(right);
  retain(retained);
  ASSERT_EQ(candy_gc_full(&collector), CANDY_OK);
  EXPECT_EQ(candy_gc_find(&collector, CANDY_TYPE_CHAR, first, len, hash), nullptr);
  EXPECT_EQ(candy_array_create_const_hashed(&collector, &context, CANDY_TYPE_CHAR, second, len, hash), right);
  release();
  ASSERT_EQ(candy_gc_full(&collector), CANDY_OK);
  EXPECT_EQ(candy_gc_find(&collector, CANDY_TYPE_CHAR, second, len, hash), nullptr);
}

TEST_F(gc, constant_arrays_intern_by_type_count_and_all_bytes) {
  const candy_integer_t data[] = {13, -8};
  const candy_integer_t changed[] = {13, -9};
  auto *array = candy_array_create_const(&collector, &context, CANDY_TYPE_INTEGER, data, 2);
  ASSERT_NE(array, nullptr);
  EXPECT_EQ(candy_object_type(reinterpret_cast<candy_object_t *>(array)), CANDY_TYPE_INTEGER);
  EXPECT_EQ(candy_array_size(array), 2U);
  EXPECT_MEMEQ(candy_array_data(array), data, sizeof(data));
  EXPECT_EQ(candy_array_create_const_hashed(&collector, &context, CANDY_TYPE_INTEGER, data, 2,
                                          hash_djb(data, sizeof(data))), array);
  EXPECT_NE(candy_array_create_const(&collector, &context, CANDY_TYPE_INTEGER, changed, 2), array);
  EXPECT_NE(candy_array_create_const(&collector, &context, CANDY_TYPE_INTEGER, data, 1), array);
  EXPECT_NE(candy_array_create_const(&collector, &context, CANDY_TYPE_CHAR, data, sizeof(data)), array);

  auto *empty = candy_array_create_const(&collector, &context, CANDY_TYPE_INTEGER, nullptr, 0);
  EXPECT_EQ(candy_array_size(empty), 0U);
  EXPECT_EQ(candy_array_create_const(&collector, &context, CANDY_TYPE_INTEGER, data, 0), empty);
  auto *nulls = candy_array_create_const(&collector, &context, CANDY_TYPE_NULL, nullptr, 2);
  EXPECT_EQ(candy_array_size(nulls), 2U);
  EXPECT_EQ(candy_array_create_const(&collector, &context, CANDY_TYPE_NULL, nullptr, 2), nulls);
  EXPECT_NE(candy_array_create_const(&collector, &context, CANDY_TYPE_NULL, nullptr, 1), nulls);
}

TEST_F(gc, constant_value_array_is_readonly_and_traces_references) {
  auto *child = reinterpret_cast<candy_object_t *>(candy_table_create(&collector, &context));
  candy_wrap_t value{};
  candy_wrap_set_object(&value, child);
  auto *array = candy_array_create_const(&collector, &context, CANDY_TYPE_WRAP, &value, 1);
  retain(reinterpret_cast<candy_object_t *>(array));

  struct mutation {
    candy_array_t *array;
    candy_gc_t *gc;
    candy_excep_t *ctx;
  } attempt{array, &collector, &context};
  EXPECT_EQ(candy_excep_try(&context, +[](void *arg) {
    auto *op = static_cast<mutation *>(arg);
    candy_array_set(op->array, op->gc, op->ctx, 0, &CANDY_WRAP_NULL);
  }, &attempt, nullptr), CANDY_ERR_ARRAY);
  EXPECT_EQ(candy_wrap_get_object(candy_array_get(array, 0)), child);

  ASSERT_EQ(candy_gc_full(&collector), CANDY_OK);
  EXPECT_EQ(objects(), 1U);
  EXPECT_EQ(collector.pool_used, 1U);
  release();
  ASSERT_EQ(candy_gc_full(&collector), CANDY_OK);
  EXPECT_EQ(objects(), 0U);
  EXPECT_EQ(collector.pool_used, 0U);
}

TEST_F(gc, constant_pool_lookup_crosses_tombstones) {
  auto *discarded = candy_array_create_const(&collector, &context, CANDY_TYPE_CHAR, "a", 1);
  auto *survivor = candy_array_create_const(&collector, &context, CANDY_TYPE_CHAR, "i", 1);
  ASSERT_NE(discarded, survivor);
  size_t mask = capacity_to_size(collector.pool_cap) - 1;
  ASSERT_EQ(hash_djb("a", 1) & mask, hash_djb("i", 1) & mask);
  auto *object = reinterpret_cast<candy_object_t *>(survivor);
  retain(object);
  ASSERT_EQ(candy_gc_full(&collector), CANDY_OK);
  EXPECT_EQ(candy_gc_find(&collector, CANDY_TYPE_CHAR, "a", 1, hash_djb("a", 1)), nullptr);
  EXPECT_EQ(candy_gc_find(&collector, CANDY_TYPE_CHAR, "i", 1, hash_djb("i", 1)), object);
  EXPECT_EQ(candy_array_create_const(&collector, &context, CANDY_TYPE_CHAR, "i", 1), survivor);
  ASSERT_EQ(candy_gc_full(&collector), CANDY_OK);
  EXPECT_EQ(candy_gc_find(&collector, CANDY_TYPE_CHAR, "i", 1, hash_djb("i", 1)), object);
  release();
  ASSERT_EQ(candy_gc_full(&collector), CANDY_OK);
  EXPECT_EQ(candy_gc_find(&collector, CANDY_TYPE_CHAR, "i", 1, hash_djb("i", 1)), nullptr);
}

TEST_F(gc, step_transitions_and_duplicate_marking) {
  candy_table_create_global(&collector, &context);
  ASSERT_EQ(candy_gc_step(&collector), CANDY_OK);
  EXPECT_EQ(candy_gc_fsm(&collector), GC_FSM_DIFFUSE);
  EXPECT_EQ(candy_object_mark(collector.glob), MARK_GRAY);
  EXPECT_EQ(candy_gc_mark(&collector, collector.glob), CANDY_OK);
  ASSERT_EQ(candy_gc_step(&collector), CANDY_OK);
  EXPECT_EQ(collector.gray, nullptr);
  EXPECT_EQ(candy_object_mark(collector.glob), MARK_DARK);
  ASSERT_EQ(candy_gc_step(&collector), CANDY_OK);
  EXPECT_EQ(candy_gc_fsm(&collector), GC_FSM_END);
  ASSERT_EQ(candy_gc_step(&collector), CANDY_OK);
  EXPECT_EQ(candy_gc_fsm(&collector), GC_FSM_BEGIN);
  EXPECT_EQ(candy_object_mark(collector.glob), MARK_WHITE);
}

TEST_F(gc, full_finishes_partial_cycle) {
  candy_table_create_global(&collector, &context);
  ASSERT_EQ(candy_gc_step(&collector), CANDY_OK);
  ASSERT_EQ(candy_gc_full(&collector), CANDY_OK);
  EXPECT_EQ(candy_gc_fsm(&collector), GC_FSM_BEGIN);
  EXPECT_EQ(collector.gray, nullptr);
}

TEST_F(gc, propagates_marking_errors) {
  candy_table_create_global(&collector, &context);
  fail = true;
  EXPECT_EQ(candy_gc_step(&collector), CANDY_ERR);
  EXPECT_EQ(candy_gc_full(&collector), CANDY_ERR);
  EXPECT_EQ(candy_gc_fsm(&collector), GC_FSM_BEGIN);
  fail = false;
  ASSERT_EQ(candy_gc_full(&collector), CANDY_OK);
}

TEST_F(gc, propagates_diffusion_errors) {
  candy_table_create_global(&collector, &context);
  ASSERT_EQ(candy_gc_step(&collector), CANDY_OK);
  failing_event = EVT_DIFFUSE;
  fail = true;
  EXPECT_EQ(candy_gc_step(&collector), CANDY_ERR);
  EXPECT_EQ(candy_gc_full(&collector), CANDY_ERR);
  EXPECT_EQ(candy_gc_fsm(&collector), GC_FSM_DIFFUSE);
  fail = false;
  ASSERT_EQ(candy_gc_full(&collector), CANDY_OK);
}

TEST_F(gc, rejects_invalid_phase) {
  collector.fsm = static_cast<candy_gc_fsm_t>(99);
  EXPECT_EQ(candy_gc_step(&collector), CANDY_ERR);
  EXPECT_EQ(candy_gc_full(&collector), CANDY_ERR);
  collector.fsm = GC_FSM_BEGIN;
}

TEST_F(gc, object_array_traces_elements_and_nulls) {
  auto *child = candy_table_create(&collector, &context);
  auto *array = candy_array_create(&collector, &context, CANDY_TYPE_TABLE);
  candy_table_t *elements[] = {child, nullptr, child};
  ASSERT_EQ(candy_array_append(array, &collector, &context, elements, 3), CANDY_OK);
  retain(reinterpret_cast<candy_object_t *>(array));
  ASSERT_EQ(candy_gc_full(&collector), CANDY_OK);
  EXPECT_EQ(objects(), 2U);
  release();
  ASSERT_EQ(candy_gc_full(&collector), CANDY_OK);
  EXPECT_EQ(objects(), 0U);
}

TEST_F(gc, object_arrays_trace_prototypes_and_closures) {
  for (candy_types_t type : {CANDY_TYPE_PROTO, CANDY_TYPE_CCLOS, CANDY_TYPE_SCLOS}) {
    SCOPED_TRACE(type);
    candy_object_t *child = nullptr;
    if (type == CANDY_TYPE_CCLOS) {
      child = reinterpret_cast<candy_object_t *>(candy_cclosure_create(
        &collector, &context, +[](candy_state_t *) -> int { return 0; }));
    } else {
      auto *proto = candy_proto_create(&collector, &context);
      child = type == CANDY_TYPE_PROTO ? reinterpret_cast<candy_object_t *>(proto)
        : reinterpret_cast<candy_object_t *>(candy_sclosure_create(&collector, &context, proto, 0));
    }
    auto *array = candy_array_create(&collector, &context, type);
    ASSERT_EQ(candy_array_append(array, &collector, &context, &child, 1), CANDY_OK);
    retain(reinterpret_cast<candy_object_t *>(array));
    for (int cycle = 0; cycle < 3; ++cycle) {
      ASSERT_EQ(candy_gc_full(&collector), CANDY_OK);
      EXPECT_EQ(objects(), type == CANDY_TYPE_SCLOS ? 3U : 2U);
      EXPECT_EQ(candy_object_mark(child), MARK_WHITE);
    }
    release();
    ASSERT_EQ(candy_gc_full(&collector), CANDY_OK);
    EXPECT_EQ(objects(), 0U);
  }
}

TEST_F(gc, object_array_self_reference_is_collected) {
  auto *array = candy_array_create(&collector, &context, CANDY_TYPE_TABLE);
  auto *object = reinterpret_cast<candy_object_t *>(array);
  ASSERT_EQ(candy_array_append(array, &collector, &context, &object, 1), CANDY_OK);
  retain(object);
  ASSERT_EQ(candy_gc_full(&collector), CANDY_OK);
  EXPECT_EQ(objects(), 1U);
  release();
  ASSERT_EQ(candy_gc_full(&collector), CANDY_OK);
  EXPECT_EQ(objects(), 0U);
}
