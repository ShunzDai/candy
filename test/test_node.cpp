/**
  * Copyright 2022-2023 ShunzDai
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
#include "gtest/gtest.h"
#include "src/candy_node.h"
#include <stdlib.h>

struct node {
  struct node *next;
  int data;
};

static struct node *_node_create(int val) {
  struct node *self = (struct node *)candy_node_create(sizeof(struct node));
  self->data = val;
  return self;
}

static int _node_delete(struct node **self) {
  return candy_node_delete((candy_node_t **)self, nullptr);
}

// static int _node_iterator(struct node **node, void *args) {
//   (*node)->data = *(int *)args;
//   return 0;
// }

TEST(queue, clear) {
  struct node *self = _node_create(sizeof(struct candy_node *));
  candy_node_add((candy_node_t *)self, (candy_node_t *)_node_create(0));
  candy_node_add((candy_node_t *)self, (candy_node_t *)_node_create(1));
  candy_node_add((candy_node_t *)self, (candy_node_t *)_node_create(2));
  candy_node_clear((candy_node_t *)self, nullptr);
  _node_delete(&self);
  EXPECT_EQ((uint64_t)self, (uint64_t)NULL);
}

TEST(queue, lifo) {
  struct node *self = _node_create(sizeof(struct candy_node *));
  candy_node_add((candy_node_t *)self, (candy_node_t *)_node_create(0));
  candy_node_add((candy_node_t *)self, (candy_node_t *)_node_create(1));
  candy_node_add((candy_node_t *)self, (candy_node_t *)_node_create(2));
  EXPECT_EQ(self->next->data, 2);
  EXPECT_EQ(self->next->next->data, 1);
  EXPECT_EQ(self->next->next->next->data, 0);
  _node_delete(&self->next);
  EXPECT_EQ(self->next->data, 1);
  EXPECT_EQ(self->next->next->data, 0);
  _node_delete(&self->next);
  EXPECT_EQ(self->next->data, 0);
  _node_delete(&self->next);
  EXPECT_EQ(candy_node_empty((candy_node_t *)self), true);
  _node_delete(&self);
  EXPECT_EQ((uint64_t)self, (uint64_t)NULL);
}

// TEST(queue, iterator) {
//   candy_node_t *queue = candy_node_create();
//   candy_enqueue(queue, -1, (candy_node_t *)_node_create(0));
//   candy_enqueue(queue, -1, (candy_node_t *)_node_create(1));
//   candy_enqueue(queue, -1, (candy_node_t *)_node_create(2));
//   int args = 114514;
//   candy_node_iterator(queue, (candy_iterator_t)_node_iterator, &args);
//   EXPECT_EQ(*((struct node *)*candy_node_pointer(queue, 0))->data, 114514);
//   EXPECT_EQ(*((struct node *)*candy_node_pointer(queue, 1))->data, 114514);
//   EXPECT_EQ(*((struct node *)*candy_node_pointer(queue, 2))->data, 114514);
//   candy_node_delete(&queue, (candy_destroy_t)_node_delete);
//   EXPECT_EQ((uint64_t)queue, (uint64_t)NULL);
// }
