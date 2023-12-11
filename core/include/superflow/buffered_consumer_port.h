// Copyright 2019, Forsvarets forskningsinstitutt. All rights reserved.
#pragma once

#include "superflow/consumer_port.h"
#include "superflow/connection_manager.h"
#include "superflow/policy.h"
#include "superflow/port.h"
#include "superflow/queue_getter.h"
#include "superflow/utils/data_stream.h"
#include "superflow/utils/lock_queue.h"

namespace flow
{
/// \brief
///
/// The port has a buffer with configurable size containing data received from the producer.
/// \tparam T The type of data to be exchanged between ports.
/// \tparam P ConnectPolicy, default is Single
/// \tparam M GetMode, default is Blocking
/// \tparam L LeakPolicy, default is Leaky
/// \tparam Variants... Optionally supported input variant types. \see ConsumerPort
template<
    typename T,
    ConnectPolicy P = ConnectPolicy::Single,
    GetMode M = GetMode::Blocking,
    LeakPolicy L = LeakPolicy::Leaky,
    typename... Variants
>
class BufferedConsumerPort final :
    public ConsumerPort<T, Variants...>,
    public DataStream<T>,
    public std::enable_shared_from_this<Port>
{
public:
  using Ptr = std::shared_ptr<BufferedConsumerPort>;
  explicit BufferedConsumerPort(unsigned int buffer_size = 1);

  void receive(const T&, const Port::Ptr&) override;

  void connect(const Port::Ptr& ptr) override;

  void disconnect() noexcept override;

  void disconnect(const Port::Ptr& ptr) noexcept override;

  bool isConnected() const override;

  std::optional<T> getNext() override;

  /// \brief Returns true if the buffer is not empty
  bool hasNext() const;

  /// \brief Check if the buffer is terminated or not
  /// \return False if the buffer is terminated.
  operator bool() const override;

  /// \brief Empties the internal buffer, any unread data will be discarded.
  void clear();

  void deactivate();

  PortStatus getStatus() const;

  size_t getQueueSize() const;

private:
  size_t num_transactions_ = 0;
  LockQueue<T, L> buffer_;
  ConnectionManager<P> connection_manager_;
  QueueGetter<T, M, L> queue_getter_;
};

// ----- Implementations -----
template<
    typename T,
    ConnectPolicy P,
    GetMode M,
    LeakPolicy L,
    typename... Variants
>
BufferedConsumerPort<T, P, M, L, Variants...>::BufferedConsumerPort(const unsigned int buffer_size)
    : buffer_(buffer_size)
{}

template<
    typename T,
    ConnectPolicy P,
    GetMode M,
    LeakPolicy L,
    typename... Variants
>
void BufferedConsumerPort<T, P, M, L, Variants...>::receive(const T& item, const Port::Ptr&)
{
  if (!buffer_.isTerminated())
  {
    try { buffer_.push(item); }
    catch(const flow::TerminatedException&) {}
  }
}

template<
    typename T,
    ConnectPolicy P,
    GetMode M,
    LeakPolicy L,
    typename... Variants
>
void BufferedConsumerPort<T, P, M, L, Variants...>::connect(const Port::Ptr& ptr)
{
  connection_manager_.connect(shared_from_this(), ptr);
}

template<
    typename T,
    ConnectPolicy P,
    GetMode M,
    LeakPolicy L,
    typename... Variants
>
void BufferedConsumerPort<T, P, M, L, Variants...>::disconnect() noexcept
{
  connection_manager_.disconnect(shared_from_this());
}

template<
    typename T,
    ConnectPolicy P,
    GetMode M,
    LeakPolicy L,
    typename... Variants
>
void BufferedConsumerPort<T, P, M, L, Variants...>::disconnect(const Port::Ptr& ptr) noexcept
{
  connection_manager_.disconnect(shared_from_this(), ptr);
}

template<
    typename T,
    ConnectPolicy P,
    GetMode M,
    LeakPolicy L,
    typename... Variants
>
bool BufferedConsumerPort<T, P, M, L, Variants...>::isConnected() const
{
  return connection_manager_.isConnected();
}

template<
    typename T,
    ConnectPolicy P,
    GetMode M,
    LeakPolicy L,
    typename... Variants
>
std::optional<T> BufferedConsumerPort<T, P, M, L, Variants...>::getNext()
{
  const auto item = queue_getter_.get(buffer_);

  if (item)
  { ++num_transactions_; }

  return item;
}

template<
    typename T,
    ConnectPolicy P,
    GetMode M,
    LeakPolicy L,
    typename... Variants
>
bool BufferedConsumerPort<T, P, M, L, Variants...>::hasNext() const
{
  return queue_getter_.hasNext(buffer_);
}

template<
    typename T,
    ConnectPolicy P,
    GetMode M,
    LeakPolicy L,
    typename... Variants
>
BufferedConsumerPort<T, P, M, L, Variants...>::operator bool() const
{
  return !buffer_.isTerminated();
}

template<
    typename T,
    ConnectPolicy P,
    GetMode M,
    LeakPolicy L,
    typename... Variants
>
PortStatus BufferedConsumerPort<T, P, M, L, Variants...>::getStatus() const
{
  return {
      connection_manager_.getNumConnections(),
      num_transactions_
  };
}

template<
    typename T,
    ConnectPolicy P,
    GetMode M,
    LeakPolicy L,
    typename... Variants
>
size_t BufferedConsumerPort<T, P, M, L, Variants...>::getQueueSize() const
{
  return buffer_.getQueueSize();
}

template<
    typename T,
    ConnectPolicy P,
    GetMode M,
    LeakPolicy L,
    typename... Variants
>
void BufferedConsumerPort<T, P, M, L, Variants...>::clear()
{
  buffer_.clearQueue();
  queue_getter_.clear();
}

template<
    typename T,
    ConnectPolicy P,
    GetMode M,
    LeakPolicy L,
    typename... Variants
>
void BufferedConsumerPort<T, P, M, L, Variants...>::deactivate()
{
  buffer_.terminate();
}
}
