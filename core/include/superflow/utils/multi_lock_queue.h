// Copyright 2019, Forsvarets forskningsinstitutt. All rights reserved.
#pragma once

#include "superflow/utils/terminated_exception.h"

#include <condition_variable>
#include <map>
#include <mutex>
#include <queue>
#include <vector>

namespace flow
{
template<typename K, typename T>
class MultiLockQueue
{
public:
  /// Creates a new MultiLockQueue where each queue has a max size of `max_queue_size`.
  /// No queues are initialized by the ctor, but will be added dynamically as you call
  /// `push()` for unused keys, or by using `addQueue()`.
  explicit MultiLockQueue(
      size_t max_queue_size
  );

  /// Creates a new MultiLockQueue where each queue has a max size of `max_queue_size`.
  /// Queues for each entry in `keys` are added by the ctor. Additional queues will be
  /// added dynamically as you call `push()` for unused keys, or by using `addQueue()`.
  MultiLockQueue(
      size_t max_queue_size,
      const std::vector<K>& keys
  );

  void push(const K& key, const T& item);

  void push(const K& key, T&& item);

  /// Returns a map of the first item in all non-empty queues,
  /// while not removing the elements. Does not block. Ever.
  /// If all queues are empty, the returned map will be empty.
  /// \throws TerminatedException if terminate() has been called
  /// prior to calling peekReady()
  std::map<K, T> peekReady() const;

  /// Returns a map of the first item in all non-empty queues,
  /// while not removing the elements. Blocks until at least one
  /// of the queues has an element. The returned map will thus
  /// contain at least one element.
  /// \throws TerminatedException if terminate() is called
  /// prior to calling peekAtLeastOne() or while waiting for data.
  std::map<K, T> peekAtLeastOne() const;

  /// Returns a map of the first item in all queues,
  /// while not removing the elements. Blocks until all queues
  /// have an element. The returned map will thus contain as many
  /// elements as there are queues.
  /// \throws TerminatedException if terminate() is called
  /// prior to calling peekAll() or while waiting for data.
  std::map<K, T> peekAll() const;

  /// Returns a map of the first item in all non-empty queues,
  /// and removes the elements from the queues. Does not block.
  /// Ever. If all queues are empty, the returned map will be
  /// empty.
  /// \throws TerminatedException if terminate() has been called
  /// prior to calling popReady()
  std::map<K, T> popReady();

  /// Returns a map of the first item in all non-empty queues,
  /// and removes the elements from the queues. Blocks until at
  /// least one of the queues has an element. The returned map
  /// will thus contain at least one element. Blocks indefinitely
  /// if there are no queues.
  /// \throws TerminatedException if terminate() is called
  /// prior to calling popAtLeastOne() or while waiting for data.
  std::map<K, T> popAtLeastOne();

  /// Returns a map of the first item in all queues,
  /// and removes the elements from the queues. Blocks until
  /// all queues have an element. The returned map will thus
  /// contain as many elements as there are queues.
  /// \throws TerminatedException if terminate() is called
  /// prior to calling popAll() or while waiting for data.
  std::map<K, T> popAll();

  void clear();

  /// Adds a new queue for `key`. Does nothing if a queue for
  /// this key already exists.
  void addQueue(const K& key);

  /// Removes the queue for `key`. Does nothing if no such queue
  /// exists
  void removeQueue(const K& key);

  void removeAllQueues();

  /// Returns true if at least one of the queues has at least one
  /// element. Does not block. Returns false if there are no queues.
  /// If it returns `true`, popAtLeastOne and peekAtLeastOne are guaranteed
  /// to return immediately.
  bool hasAny() const;

  /// Returns true if all queues have at least one element.
  /// Returns true if there are no queues.
  /// If it returns `true`, all pop- and peek-methods are guaranteed
  /// to return immediately.
  bool hasAll() const;

  /// Sets the terminated flag and causes all
  /// calls to pop- and peek-methods to throw TerminatedException.
  /// This also happens to preexisting calls that are blocking while
  /// waiting for data.
  void terminate();

  /// Returns true if queue has been terminated. When true
  /// all calls to pop- and peek-methods will throw TerminatedException.
  bool isTerminated() const;

  /// Returns the number of queues.
  size_t getNumQueues() const;

private:
  mutable std::mutex mutex_;
  mutable std::condition_variable cond_;

  std::map<K, std::queue<T>> queues_;
  size_t max_queue_size_;
  bool terminated_;

  std::unique_lock<std::mutex> waitAny() const;

  std::unique_lock<std::mutex> waitAll() const;

  std::map<K, T> peekReady(const std::unique_lock<std::mutex>& lock) const;

  std::map<K, T> popReady(const std::unique_lock<std::mutex>& lock);

  bool hasAny(const std::unique_lock<std::mutex>& lock) const;

  bool hasAll(const std::unique_lock<std::mutex>& lock) const;

  static std::map<K, std::queue<T>> createQueues(const std::vector<K>& keys);
};

// ----- Implementation -----
template<typename K, typename T>
MultiLockQueue<K, T>::MultiLockQueue(const size_t max_queue_size)
    : MultiLockQueue{max_queue_size, {}}
{}

template<typename K, typename T>
MultiLockQueue<K, T>::MultiLockQueue(
    const size_t max_queue_size,
    const std::vector<K>& keys
)
    : queues_{createQueues(keys)}
    , max_queue_size_{max_queue_size}
    , terminated_{false}
{}

template<typename K, typename T>
void MultiLockQueue<K, T>::push(const K& key, const T& item)
{
  {
    std::lock_guard<std::mutex> lock{mutex_};

    auto& queue = queues_[key];

    if (queue.size() >= max_queue_size_)
    {
      queue.pop();
    }

    queue.push(item);
  }

  cond_.notify_one();
}

template<typename K, typename T>
void MultiLockQueue<K, T>::push(const K& key, T&& item)
{
  {
    std::lock_guard<std::mutex> lock{mutex_};

    auto& queue = queues_[key];

    if (queue.size() >= max_queue_size_)
    {
      queue.pop();
    }

    queue.push(std::move(item));
  }

  cond_.notify_one();
}

template<typename K, typename T>
std::map<K, T> MultiLockQueue<K, T>::peekReady() const
{
  if (terminated_)
  {
    throw TerminatedException();
  }

  const std::unique_lock<std::mutex> lock{mutex_};

  return peekReady(lock);
}

template<typename K, typename T>
std::map<K, T> MultiLockQueue<K, T>::peekAtLeastOne() const
{
  const auto& lock = waitAny();

  return peekReady(lock);
}

template<typename K, typename T>
std::map<K, T> MultiLockQueue<K, T>::peekAll() const
{
  const auto& lock = waitAll();

  return peekReady(lock);
}

template<typename K, typename T>
std::map<K, T> MultiLockQueue<K, T>::popReady()
{
  if (terminated_)
  {
    throw TerminatedException();
  }

  const std::unique_lock<std::mutex> lock{mutex_};

  return popReady(lock);
}

template<typename K, typename T>
std::map<K, T> MultiLockQueue<K, T>::popAtLeastOne()
{
  const auto& lock = waitAny();

  return popReady(lock);
}

template<typename K, typename T>
std::map<K, T> MultiLockQueue<K, T>::popAll()
{
  const auto& lock = waitAll();

  return popReady(lock);
}

template<typename K, typename T>
void MultiLockQueue<K, T>::clear()
{
  std::lock_guard<std::mutex> lock{mutex_};

  for (auto& kv : queues_)
  {
    auto& queue = kv.second;

    std::queue<T> empty_queue;
    std::swap(queue, empty_queue);
  }
}

template<typename K, typename T>
void MultiLockQueue<K, T>::addQueue(const K& key)
{
  std::lock_guard<std::mutex> lock{mutex_};

  if (queues_.find(key) != queues_.end())
  {
    return;
  }

  queues_[key] = std::queue<T>{};
}

template<typename K, typename T>
void MultiLockQueue<K, T>::removeQueue(const K& key)
{
  std::lock_guard<std::mutex> lock{mutex_};

  queues_.erase(key);
}

template<typename K, typename T>
void MultiLockQueue<K, T>::removeAllQueues()
{
  std::lock_guard<std::mutex> lock{mutex_};

  queues_.clear();
}

template<typename K, typename T>
bool MultiLockQueue<K, T>::hasAny() const
{
  const std::unique_lock<std::mutex> lock{mutex_};

  return hasAny(lock);
}

template<typename K, typename T>
bool MultiLockQueue<K, T>::hasAny(const std::unique_lock<std::mutex>&) const
{
  for (const auto& kv : queues_)
  {
    const auto& queue = kv.second;

    if (!queue.empty())
    {
      return true;
    }
  }

  return false;
}

template<typename K, typename T>
bool MultiLockQueue<K, T>::hasAll() const
{
  const std::unique_lock<std::mutex> lock{mutex_};

  return hasAll(lock);
}

template<typename K, typename T>
bool MultiLockQueue<K, T>::hasAll(const std::unique_lock<std::mutex>&) const
{
  for (const auto& kv : queues_)
  {
    const auto& queue = kv.second;

    if (queue.empty())
    {
      return false;
    }
  }

  return true;
}

template<typename K, typename T>
void MultiLockQueue<K, T>::terminate()
{
  if (terminated_)
  {
    return;
  }

  terminated_ = true;
  cond_.notify_all();
}

template<typename K, typename T>
bool MultiLockQueue<K, T>::isTerminated() const
{
  return terminated_;
}

template<typename K, typename T>
size_t MultiLockQueue<K, T>::getNumQueues() const
{
  std::lock_guard<std::mutex> lock{mutex_};

  return queues_.size();
}

template<typename K, typename T>
std::unique_lock<std::mutex> MultiLockQueue<K, T>::waitAny() const
{
  std::unique_lock<std::mutex> lock{mutex_};

  cond_.wait(
      lock,
      [this, &lock]()
      {
        return terminated_ || hasAny(lock);
      }
  );

  if (terminated_)
  {
    throw TerminatedException();
  }

  return lock;
}

template<typename K, typename T>
std::unique_lock<std::mutex> MultiLockQueue<K, T>::waitAll() const
{
  std::unique_lock<std::mutex> lock{mutex_};

  cond_.wait(
      lock,
      [this, &lock]()
      {
        return terminated_ || hasAll(lock);
      }
  );

  if (terminated_)
  {
    throw TerminatedException();
  }

  return lock;
}

template<typename K, typename T>
std::map<K, T> MultiLockQueue<K, T>::peekReady(const std::unique_lock<std::mutex>&) const
{
  std::map<K, T> items;

  for (const auto& kv : queues_)
  {
    const auto& key = kv.first;
    const auto& queue = kv.second;

    if (!queue.empty())
    {
      items[key] = queue.front();
    }
  }

  return items;
}

template<typename K, typename T>
std::map<K, T> MultiLockQueue<K, T>::popReady(const std::unique_lock<std::mutex>&)
{
  std::map<K, T> items;

  for (auto& kv : queues_)
  {
    const auto& key = kv.first;
    auto& queue = kv.second;

    if (!queue.empty())
    {
      items[key] = queue.front();
      queue.pop();
    }
  }

  return items;
}

template<typename K, typename T>
std::map<K, std::queue<T>> MultiLockQueue<K, T>::createQueues(const std::vector<K>& keys)
{
  std::map<K, std::queue<T>> queues;

  for (const auto& key : keys)
  {
    queues[key] = std::queue<T>{};
  }

  return queues;
}
}
