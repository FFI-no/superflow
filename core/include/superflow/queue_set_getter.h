// Copyright 2019, Forsvarets forskningsinstitutt. All rights reserved.
#pragma once

#include "superflow/policy.h"
#include "superflow/queue_getter.h"
#include "superflow/utils/lock_queue.h"

#include <memory>
#include <vector>

namespace flow
{
template<typename T, GetMode M>
struct QueueSetGetter
{
  using Queue = LockQueue<T>;
  using QueueSet = std::vector<std::shared_ptr<Queue>>;

  static void get(const QueueSet& queues, std::vector<T>& ts)
  {
    ts.resize(queues.size());

    for (size_t i = 0; i < queues.size(); ++i)
    {
      QueueGetter<T, M>::get(*queues[i], ts[i]);
    }
  }

  static bool hasNext(const QueueSet& queues)
  {
    for (const auto& queue : queues)
    {
      if (queue->getQueueSize() == 0)
      {
        return false;
      }
    }

    return true;
  }
};

template<typename T>
struct QueueSetGetter<T, GetMode::ReadyOnly>
{
  using Queue = LockQueue<T>;
  using QueueSet = std::vector<std::shared_ptr<Queue>>;

  static void get(const QueueSet& queues, std::vector<T>& ts)
  {
    QueueSet ready_queues;

    for (const auto& queue : queues)
    {
      if (queue->getQueueSize() > 0)
      {
        ready_queues.push_back(queue);
      }
    }

    ts.resize(ready_queues.size());

    for (size_t i = 0; i < ready_queues.size(); ++i)
    {
      QueueGetter<T, GetMode::Blocking>::get(*ready_queues[i], ts[i]);
    }
  }

  static bool hasNext(const QueueSet& queues)
  {
    for (const auto& queue : queues)
    {
      if (queue->getQueueSize() > 0)
      {
        return true;
      }
    }

    return false;
  }
};
}
