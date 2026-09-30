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
    ASSERT_EQ(candy_gc_init(&collector, &context, handler, test_allocator, this), CANDY_OK);
    EXPECT_EQ(collector.threshold, CANDY_CONFIG_GC_THRESHOLD);
    EXPECT_EQ(collector.prim, nullptr);
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
  auto *closure = candy_sclosure_create(&collector, &context, proto);
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
        : reinterpret_cast<candy_object_t *>(candy_sclosure_create(&collector, &context, proto));
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
