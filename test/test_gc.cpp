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
#include "test.h"
#include "core/candy_object.h"
#include "core/candy_gc.h"

struct object_stub0 {
  candy_object_t header;
};

struct object_stub1 {
  candy_object_t header;
  candy_object_t *gray;
  candy_object_t *stub;
};

static object_stub0 *_object_stub0_create(candy_gc_t *gc) {
  auto self = (object_stub0 *)candy_gc_add(gc, nullptr, CANDY_TYPE_STUB0, sizeof(object_stub0));
  return self;
}

static int _object_stub0_delete(object_stub0 *self, candy_gc_t *gc) {
  candy_gc_free(gc, self, sizeof(object_stub0));
  return 0;
}

static int _object_stub0_colouring(object_stub0 *self, candy_gc_t *gc) {
  candy_object_set_mark((candy_object_t *)self, MARK_DARK);
  return 0;
}

static int _object_stub0_diffusion(object_stub0 *self, candy_gc_t *gc) {
  return 0;
}

static object_stub1 *_object_stub1_create(candy_gc_t *gc) {
  auto self = (object_stub1 *)candy_gc_add(gc, nullptr, CANDY_TYPE_STUB1, sizeof(object_stub1));
  self->gray = nullptr;
  self->stub = nullptr;
  return self;
}

static int _object_stub1_delete(object_stub1 *self, candy_gc_t *gc) {
  candy_gc_free(gc, self, sizeof(object_stub1));
  return 0;
}

static int _object_stub1_colouring(object_stub1 *self, candy_gc_t *gc) {
  self->gray = candy_gc_gray_swap(gc, (candy_object_t *)self);
  candy_object_set_mark((candy_object_t *)self, MARK_GRAY);
  return 0;
}

static int _object_stub1_diffusion(object_stub1 *self, candy_gc_t *gc) {
  candy_gc_gray_swap(gc, self->gray);
  candy_object_set_mark((candy_object_t *)self, MARK_DARK);
  candy_gc_event_handler(gc)((candy_object_t *)self->stub, gc, EVT_COLOURING);
  return 0;
}

static int _event_delete(candy_object_t *self, candy_gc_t *gc) {
  switch (candy_object_get_type(self)) {
    case CANDY_TYPE_STUB0: return _object_stub0_delete((object_stub0 *)self, gc);
    case CANDY_TYPE_STUB1: return _object_stub1_delete((object_stub1 *)self, gc);
    default:               return -1;
  }
}

static int _event_colouring(candy_object_t *self, candy_gc_t *gc) {
  switch (candy_object_get_type(self)) {
    case CANDY_TYPE_STUB0: return _object_stub0_colouring((object_stub0 *)self, gc);
    case CANDY_TYPE_STUB1: return _object_stub1_colouring((object_stub1 *)self, gc);
    default:               return -1;
  }
}

static int _event_diffusion(candy_object_t *self, candy_gc_t *gc) {
  switch (candy_object_get_type(self)) {
    case CANDY_TYPE_STUB0: return _object_stub0_diffusion((object_stub0 *)self, gc);
    case CANDY_TYPE_STUB1: return _object_stub1_diffusion((object_stub1 *)self, gc);
    default:               return -1;
  }
}

static int handler(candy_object_t *self, candy_gc_t *gc, candy_events_t evt) {
  switch (evt) {
    case EVT_DELETE:    return _event_delete(self, gc);
    case EVT_COLOURING: return _event_colouring(self, gc);
    case EVT_DIFFUSION: return _event_diffusion(self, gc);
    default:            return -1;
  }
}

TEST(gc, unique_name(colouring)) {
  candy_gc_t gc{};
  candy_gc_init(&gc, handler, test_allocator, nullptr);
  auto main = _object_stub1_create(&gc);
  candy_gc_move(&gc, GC_MV_MAIN);
  EXPECT_TRUE(candy_gc_main(&gc) == (candy_object_t *)main);

  auto obj0 = (candy_object_t *)_object_stub0_create(&gc);
  auto obj1 = (candy_object_t *)_object_stub0_create(&gc);

  main->stub = obj0;

  EXPECT_EQ(candy_gc_fsm(&gc), GC_FSM_BEGIN);
  ASSERT_EQ(gc.pool, obj1);
  ASSERT_EQ(*candy_object_get_next(gc.pool), obj0);
  ASSERT_EQ(*candy_object_get_next(*candy_object_get_next(gc.pool)), nullptr);
  EXPECT_EQ(candy_object_get_mark((candy_object_t *)main), MARK_WHITE);
  EXPECT_EQ(candy_object_get_mark(obj0), MARK_WHITE);
  EXPECT_EQ(candy_object_get_mark(obj1), MARK_WHITE);

  candy_gc_step(&gc);

  EXPECT_EQ(candy_gc_fsm(&gc), GC_FSM_DIFFUSION);
  ASSERT_EQ(gc.pool, obj1);
  ASSERT_EQ(*candy_object_get_next(gc.pool), obj0);
  ASSERT_EQ(*candy_object_get_next(*candy_object_get_next(gc.pool)), nullptr);
  EXPECT_EQ(candy_object_get_mark((candy_object_t *)main), MARK_GRAY);
  EXPECT_EQ(candy_object_get_mark(obj0), MARK_WHITE);
  EXPECT_EQ(candy_object_get_mark(obj1), MARK_WHITE);

  candy_gc_step(&gc);

  EXPECT_EQ(candy_gc_fsm(&gc), GC_FSM_DIFFUSION);
  ASSERT_EQ(gc.pool, obj1);
  ASSERT_EQ(*candy_object_get_next(gc.pool), obj0);
  ASSERT_EQ(*candy_object_get_next(*candy_object_get_next(gc.pool)), nullptr);
  EXPECT_EQ(candy_object_get_mark((candy_object_t *)main), MARK_DARK);
  EXPECT_EQ(candy_object_get_mark(obj0), MARK_DARK);
  EXPECT_EQ(candy_object_get_mark(obj1), MARK_WHITE);

  candy_gc_step(&gc);

  EXPECT_EQ(candy_gc_fsm(&gc), GC_FSM_END);
  ASSERT_EQ(gc.pool, obj1);
  ASSERT_EQ(*candy_object_get_next(gc.pool), obj0);
  ASSERT_EQ(*candy_object_get_next(*candy_object_get_next(gc.pool)), nullptr);
  EXPECT_EQ(candy_object_get_mark((candy_object_t *)main), MARK_DARK);
  EXPECT_EQ(candy_object_get_mark(obj0), MARK_DARK);
  EXPECT_EQ(candy_object_get_mark(obj1), MARK_WHITE);

  candy_gc_step(&gc);

  EXPECT_EQ(candy_gc_fsm(&gc), GC_FSM_BEGIN);
  ASSERT_EQ(gc.pool, obj0);
  ASSERT_EQ(*candy_object_get_next(gc.pool), nullptr);
  EXPECT_EQ(candy_object_get_mark((candy_object_t *)main), MARK_DARK);
  EXPECT_EQ(candy_object_get_mark(obj0), MARK_WHITE);

  candy_gc_deinit(&gc);
}
