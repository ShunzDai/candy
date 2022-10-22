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
#include "gtest/gtest.h"
#include "src/struct/candy_queue.h"
#include <stdlib.h>

struct node {
  struct node *next;
  int *data;
};

static struct node *_node_create(int val) {
  struct node *node = (struct node *)malloc(sizeof(struct node));
  node->next = NULL;
  node->data = (int *)malloc(sizeof(int));
  *node->data = val;
  return node;
}

static int _node_delete(struct node **node) {
  free((*node)->data);
  free(*node);
  return 0;
}

static void _node_iterator(struct node **node, void *args) {
  *(*node)->data = *(int *)args;
}

TEST(queue, pointer) {
  struct candy_queue *queue = candy_queue_create();
  EXPECT_EQ((uint64_t)candy_queue_pointer(queue, 0), (uint64_t)&queue->head);
  candy_enqueue(queue, -1, (struct candy_node *)_node_create(0));
  candy_enqueue(queue, -1, (struct candy_node *)_node_create(1));
  candy_enqueue(queue, -1, (struct candy_node *)_node_create(2));
  EXPECT_EQ((uint64_t)candy_queue_pointer(queue, -3), (uint64_t)&queue->head->next);
  EXPECT_EQ((uint64_t)candy_queue_pointer(queue, -2), (uint64_t)&queue->head->next->next);
  EXPECT_EQ((uint64_t)candy_queue_pointer(queue, -1), (uint64_t)&queue->head->next->next->next);
  EXPECT_EQ((uint64_t)candy_queue_pointer(queue, 0), (uint64_t)&queue->head);
  EXPECT_EQ((uint64_t)candy_queue_pointer(queue, 1), (uint64_t)&queue->head->next);
  EXPECT_EQ((uint64_t)candy_queue_pointer(queue, 2), (uint64_t)&queue->head->next->next);
  EXPECT_EQ((uint64_t)candy_queue_pointer(queue, 3), (uint64_t)&queue->head->next->next->next);
  EXPECT_EQ((uint64_t)*candy_queue_pointer(queue, 3), (uint64_t)NULL);
  candy_queue_delete(&queue, (candy_destroy_t)_node_delete);
  EXPECT_EQ((uint64_t)queue, (uint64_t)NULL);
}

TEST(queue, clear) {
  struct candy_queue *queue = candy_queue_create();
  candy_enqueue(queue, -1, (struct candy_node *)_node_create(0));
  candy_enqueue(queue, -1, (struct candy_node *)_node_create(1));
  candy_enqueue(queue, -1, (struct candy_node *)_node_create(2));
  EXPECT_EQ(candy_queue_size(queue), 3);
  candy_queue_clear(queue, (candy_destroy_t)_node_delete);
  EXPECT_EQ(candy_queue_size(queue), 0);
  candy_queue_delete(&queue, (candy_destroy_t)_node_delete);
  EXPECT_EQ((uint64_t)queue, (uint64_t)NULL);
}

TEST(queue, fifo) {
  struct candy_queue *queue = candy_queue_create();
  candy_enqueue(queue, -1, (struct candy_node *)_node_create(0));
  candy_enqueue(queue, -1, (struct candy_node *)_node_create(1));
  candy_enqueue(queue, -1, (struct candy_node *)_node_create(2));
  EXPECT_EQ(*((struct node *)*candy_queue_pointer(queue, 0))->data, 0);
  EXPECT_EQ(*((struct node *)*candy_queue_pointer(queue, 1))->data, 1);
  EXPECT_EQ(*((struct node *)*candy_queue_pointer(queue, 2))->data, 2);
  candy_dequeue(queue, 0, (candy_destroy_t)_node_delete);
  EXPECT_EQ(*((struct node *)*candy_queue_pointer(queue, 0))->data, 1);
  EXPECT_EQ(*((struct node *)*candy_queue_pointer(queue, 1))->data, 2);
  candy_dequeue(queue, 0, (candy_destroy_t)_node_delete);
  EXPECT_EQ(*((struct node *)*candy_queue_pointer(queue, 0))->data, 2);
  candy_dequeue(queue, 0, (candy_destroy_t)_node_delete);
  EXPECT_EQ(candy_queue_empty(queue), true);
  candy_queue_delete(&queue, (candy_destroy_t)_node_delete);
  EXPECT_EQ((uint64_t)queue, (uint64_t)NULL);
}

TEST(queue, lifo) {
  struct candy_queue *queue = candy_queue_create();
  candy_enqueue(queue, 0, (struct candy_node *)_node_create(0));
  candy_enqueue(queue, 0, (struct candy_node *)_node_create(1));
  candy_enqueue(queue, 0, (struct candy_node *)_node_create(2));
  EXPECT_EQ(*((struct node *)*candy_queue_pointer(queue, 0))->data, 2);
  EXPECT_EQ(*((struct node *)*candy_queue_pointer(queue, 1))->data, 1);
  EXPECT_EQ(*((struct node *)*candy_queue_pointer(queue, 2))->data, 0);
  candy_dequeue(queue, 0, (candy_destroy_t)_node_delete);
  EXPECT_EQ(*((struct node *)*candy_queue_pointer(queue, 0))->data, 1);
  EXPECT_EQ(*((struct node *)*candy_queue_pointer(queue, 1))->data, 0);
  candy_dequeue(queue, 0, (candy_destroy_t)_node_delete);
  EXPECT_EQ(*((struct node *)*candy_queue_pointer(queue, 0))->data, 0);
  candy_dequeue(queue, 0, (candy_destroy_t)_node_delete);
  EXPECT_EQ(candy_queue_empty(queue), true);
  candy_queue_delete(&queue, (candy_destroy_t)_node_delete);
  EXPECT_EQ((uint64_t)queue, (uint64_t)NULL);
}

TEST(queue, iterator) {
  struct candy_queue *queue = candy_queue_create();
  candy_enqueue(queue, -1, (struct candy_node *)_node_create(0));
  candy_enqueue(queue, -1, (struct candy_node *)_node_create(1));
  candy_enqueue(queue, -1, (struct candy_node *)_node_create(2));
  int args = 114514;
  candy_queue_iterator(queue, (candy_iterator_t)_node_iterator, &args);
  EXPECT_EQ(*((struct node *)*candy_queue_pointer(queue, 0))->data, 114514);
  EXPECT_EQ(*((struct node *)*candy_queue_pointer(queue, 1))->data, 114514);
  EXPECT_EQ(*((struct node *)*candy_queue_pointer(queue, 2))->data, 114514);
  candy_queue_delete(&queue, (candy_destroy_t)_node_delete);
  EXPECT_EQ((uint64_t)queue, (uint64_t)NULL);
}
