// Copyright 2019, Forsvarets forskningsinstitutt. All rights reserved.
#pragma once

#include "superflow/policy.h"

#include "superflow/utils/multi_lock_queue.h"

#include <map>

namespace flow
{
template<typename K, typename T, GetMode M>
class MultiQueueGetter
{};

template<typename K, typename T>
class MultiQueueGetter<K, T, GetMode::Blocking>
{
public:
  void get(MultiLockQueue<K, T>& multi_queue, std::vector<T>& items)
  {
    auto item_map = multi_queue.popAll();

    items.resize(item_map.size());

    size_t i = 0;

    for (auto& kv : item_map)
    {
      items[i++] = std::move(kv.second);
    }
  }

  bool hasNext(const MultiLockQueue<K, T>& multi_queue) const
  {
    return multi_queue.hasAll();
  }
};

template<typename K, typename T>
class MultiQueueGetter<K, T, GetMode::Latched>
{
public:
  void get(MultiLockQueue<K, T>& multi_queue, std::vector<T>& items)
  {
    if (last_items_.empty())
    {
      last_items_ = multi_queue.popAll();
    } else
    {
      for (auto& kv : multi_queue.popReady())
      {
        last_items_[kv.first] = std::move(kv.second);
      }
    }

    items.resize(last_items_.size());

    size_t i = 0;

    for (const auto& kv : last_items_)
    {
      items[i++] = kv.second;
    }
  }

  bool hasNext(const MultiLockQueue<K, T>& multi_queue) const
  {
    if (last_items_.empty())
    {
      return multi_queue.hasAll();
    }

    return true;
  }

private:
  std::map<K, T> last_items_;
};

template<typename K, typename T>
class MultiQueueGetter<K, T, GetMode::ReadyOnly>
{
public:
  void get(MultiLockQueue<K, T>& multi_queue, std::vector<T>& items)
  {
    auto item_map = multi_queue.popReady();
    items.resize(item_map.size());

    size_t i = 0;

    for (auto& kv : item_map)
    {
      items[i++] = std::move(kv.second);
    }
  }

  bool hasNext(const MultiLockQueue<K, T>& multi_queue) const
  {
    return true;
  }

private:
  std::map<K, T> last_items_;
};

template<typename K, typename T>
class MultiQueueGetter<K, T, GetMode::AtLeastOneNew>
{
public:
  void get(MultiLockQueue<K, T>& multi_queue, std::vector<T>& items)
  {
    if (last_items_.empty())
    {
      last_items_ = multi_queue.popAll();
    } else
    {
      for (auto& kv : multi_queue.popAtLeastOne())
      {
        last_items_[kv.first] = std::move(kv.second);
      }
    }

    items.resize(last_items_.size());

    size_t i = 0;

    for (const auto& kv : last_items_)
    {
      items[i++] = kv.second;
    }
  }

  bool hasNext(const MultiLockQueue<K, T>& multi_queue) const
  {
    if (last_items_.empty())
    {
      return multi_queue.hasAll();
    }

    return multi_queue.hasAny();
  }

private:
  std::map<K, T> last_items_;
};
}
