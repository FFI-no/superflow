// Copyright 2019, Forsvarets forskningsinstitutt. All rights reserved.
#pragma once

#include "superflow/policy.h"
#include "superflow/utils/terminated_exception.h"

#include <condition_variable>
#include <mutex>
#include <queue>

namespace flow
{
template<typename T, LeakPolicy L = LeakPolicy::Leaky>
class LockQueue
{
public:
  explicit LockQueue(unsigned int max_queue_size);

  LockQueue(unsigned int max_queue_size, std::initializer_list<T> list);

  ~LockQueue();

  void clearQueue();

  [[nodiscard]] size_t getQueueSize() const;

  [[nodiscard]] bool isEmpty() const;

  [[nodiscard]] bool isTerminated() const;

  void terminate();

  void push(const T& item);

  void push(T&& item);

  void front(T& t) const;

  [[nodiscard]] T front() const;

  T pop();

  void pop(T&);

private:
  mutable std::mutex mutex_;
  mutable std::condition_variable consumer_;
  mutable std::condition_variable producer_;

  std::queue<T> queue_;
  const unsigned long max_queue_size_ = 1;
  bool terminated_ = false;

  std::unique_lock<std::mutex> consumerWait() const;

  std::unique_lock<std::mutex> producerWait() const;

  void consumerSatisfied() const;

  void producerSatisfied() const;
};

// ----- Implementations
template<typename T, LeakPolicy L>
LockQueue<T, L>::LockQueue(unsigned int max_queue_size)
  : LockQueue(max_queue_size, {})
{}

template<typename T, LeakPolicy L>
LockQueue<T, L>::LockQueue(unsigned int max_queue_size, std::initializer_list<T> list)
  : queue_{list}
  , max_queue_size_(max_queue_size)
{
  if (max_queue_size_ < 1)
  { throw std::invalid_argument("LockQueue ctor: argument 'max_queue_size' must be 1 or more."); }

  if (queue_.size() > max_queue_size)
  { throw std::range_error("initializer list contains more than 'max_queue_size' elements"); }
}

template<typename T, LeakPolicy L>
LockQueue<T, L>::~LockQueue()
{ terminate(); }

template<typename T, LeakPolicy L>
void LockQueue<T, L>::clearQueue()
{
  std::lock_guard<std::mutex> mlock(mutex_);
  std::queue<T> empty;
  std::swap(queue_, empty);
}

template<typename T, LeakPolicy L>
size_t LockQueue<T, L>::getQueueSize() const
{
  std::lock_guard<std::mutex> lock{mutex_};
  return queue_.size();
}

template<typename T, LeakPolicy L>
bool LockQueue<T, L>::isEmpty() const
{
  std::lock_guard<std::mutex> lock{mutex_};
  return queue_.empty();
}

template<typename T, LeakPolicy L>
bool LockQueue<T, L>::isTerminated() const
{ return terminated_; }

template<typename T, LeakPolicy L>
void LockQueue<T, L>::terminate()
{
  if (terminated_)
  { return; }

  terminated_ = true;
  producer_.notify_all();
  consumer_.notify_all();
}

template<typename T, LeakPolicy L>
void LockQueue<T, L>::front(T& t) const
{
  const auto mlock = consumerWait();

  t = queue_.front();
}

template<typename T, LeakPolicy L>
T LockQueue<T, L>::front() const
{
  const auto mlock = consumerWait();

  return queue_.front();
}

template<typename T, LeakPolicy L>
T LockQueue<T, L>::pop()
{
  auto mlock = consumerWait();

  T item = std::move(queue_.front());
  queue_.pop();

  mlock.unlock();
  consumerSatisfied();
  return item;
}

template<typename T, LeakPolicy L>
void LockQueue<T, L>::pop(T& item)
{
  auto mlock = consumerWait();

  std::swap(item, queue_.front());
  queue_.pop();
  mlock.unlock();
  consumerSatisfied();
}

template<typename T, LeakPolicy L>
void LockQueue<T, L>::push(T&& item)
{
  auto mlock = producerWait();

  if (queue_.size() >= max_queue_size_)
  { queue_.pop(); }

  queue_.push(std::move(item));
  mlock.unlock();
  producerSatisfied();
}

template<typename T, LeakPolicy L>
void LockQueue<T, L>::push(const T& item)
{
  auto mlock = producerWait();

  if (queue_.size() >= max_queue_size_)
  { queue_.pop(); }

  queue_.push(item);
  mlock.unlock();
  producerSatisfied();
}

template<typename T, LeakPolicy L>
std::unique_lock<std::mutex> LockQueue<T, L>::consumerWait() const
{
  std::unique_lock<std::mutex> mlock(mutex_);
  consumer_.wait(mlock, [this](){ return !queue_.empty() || terminated_; });

  if (terminated_)
  { throw TerminatedException(); }

  return mlock;
}

template<typename T, LeakPolicy L>
std::unique_lock<std::mutex> LockQueue<T, L>::producerWait() const
{
  std::unique_lock<std::mutex> mlock(mutex_);

  if constexpr (L == LeakPolicy::PushBlocking)
  {
    producer_.wait(mlock, [this]()
    { return queue_.size() < max_queue_size_ || terminated_; });
  }

  if (terminated_)
  { throw TerminatedException(); }

  return mlock;
}

template<typename T, LeakPolicy L>
void LockQueue<T, L>::consumerSatisfied() const
{
  if constexpr (L == LeakPolicy::PushBlocking)
  { producer_.notify_one(); }

  consumer_.notify_one();
}

template<typename T, LeakPolicy L>
void LockQueue<T, L>::producerSatisfied() const
{
  consumer_.notify_one();
}
}
