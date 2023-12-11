// Copyright 2019, Forsvarets forskningsinstitutt. All rights reserved.
#include "superflow/multi_queue_getter.h"

#include "gtest/gtest.h"

using namespace flow;

TEST(MultiQueueGetter, LatchedPopsQueuesWithMultipleElements)
{
  MultiLockQueue<int, int> multi_queue(2, {0, 1});
  MultiQueueGetter<int, int, GetMode::Latched> getter;

  // push elements `42` and `13` into queue 0
  multi_queue.push(0, 42);
  multi_queue.push(0, 13);

  // only push element `42` into queue 1
  multi_queue.push(1, 42);

  {
    std::vector<int> values;
    getter.get(multi_queue, values);

    // expect both to be 42
    ASSERT_EQ(values[0], 42);
    ASSERT_EQ(values[1], 42);
  }

  {
    std::vector<int> values;
    getter.get(multi_queue, values);

    // expect one value (the one from queue 0) to be 13, and the other to be 42
    // QueueGetter does not guarantee the order, so we don't know which is which
    ASSERT_EQ(std::max(values[0], values[1]), 42);
    ASSERT_EQ(std::min(values[0], values[1]), 13);
  }

  {
    std::vector<int> values;
    getter.get(multi_queue, values);

    // still expect one value (the one from queue 0) to be 13, and the other to be 42
    ASSERT_EQ(std::max(values[0], values[1]), 42);
    ASSERT_EQ(std::min(values[0], values[1]), 13);
  }
}
