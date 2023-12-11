// Copyright 2019, Forsvarets forskningsinstitutt. All rights reserved.
#pragma once

#include "superflow/consumer_port.h"
#include "superflow/connection_manager.h"
#include "superflow/policy.h"
#include "superflow/port.h"
#include "superflow/multi_queue_getter.h"
#include "superflow/utils/data_stream.h"
#include "superflow/utils/multi_lock_queue.h"

#include <map>
#include <vector>

namespace flow
{
/// \brief The port has one buffer for each connected producer.
/// Data from the the producers are received in a vector with a size depending on the GetMode selected.
/// \tparam T The type of data consumed
/// \tparam M The GetMode, defining the behavior of the port.
/// \see GetMode
/// \tparam Variants... Optionally supported input variant types. \see ConsumerPort
template<
    typename T,
    GetMode M = GetMode::Blocking,
    typename... Variants
>
class MultiConsumerPort final :
    public ConsumerPort<T, Variants...>,
    public DataStream<std::vector<T>>,
    public std::enable_shared_from_this<Port>
{
public:
  using Ptr = std::shared_ptr<MultiConsumerPort>;
  /// \brief Create a new MultiConsumerPort
  /// \param buffer_size Number of elements in each buffer.
  explicit MultiConsumerPort(
      size_t buffer_size = 1
  );

  void receive(const T&, const Port::Ptr&) override;

  void connect(const Port::Ptr& ptr) override;

  void disconnect() noexcept override;

  void disconnect(const Port::Ptr& ptr) noexcept override;

  bool isConnected() const override;

  PortStatus getStatus() const override;

  std::optional<std::vector<T>> getNext() override;

  /// \brief Get new elements from the buffer
  /// \return New elements. Number of elements depends on the selected GetMode.
  std::vector<T> get();

  /// \brief Test if any buffer has unconsumed data.
  /// \return true if new data is available.
  bool hasNext() const;

  operator bool() const override;

  /// \brief Empties the internal buffers, any unread data will be discarded.
  void clear();

  /// \brief Deactivate the port, i.e. terminate all buffers.
  void deactivate();

private:
  size_t num_transactions_ = 0;

  ConnectionManager<ConnectPolicy::Multi> connection_manager_;
  MultiLockQueue<Port::Ptr, T> multi_queue_;
  MultiQueueGetter<Port::Ptr, T, M> queue_getter_;
};

// ----- Implementation -----
template<
  typename T,
  GetMode M,
  typename... Variants
>
MultiConsumerPort<T, M, Variants...>::MultiConsumerPort(
    const size_t buffer_size
)
    : multi_queue_{buffer_size}
{}

template<
  typename T,
  GetMode M,
  typename... Variants
>
inline void MultiConsumerPort<T, M, Variants...>::receive(const T& t, const Port::Ptr& ptr)
{
  multi_queue_.push(ptr, t);
}

template<
  typename T,
  GetMode M,
  typename... Variants
>
void MultiConsumerPort<T, M, Variants...>::connect(const Port::Ptr& ptr)
{
  connection_manager_.connect(shared_from_this(), ptr);
  multi_queue_.addQueue(ptr);
}

template<
  typename T,
  GetMode M,
  typename... Variants
>
void MultiConsumerPort<T, M, Variants...>::disconnect() noexcept
{
  connection_manager_.disconnect(shared_from_this());
  multi_queue_.removeAllQueues();
}

template<
  typename T,
  GetMode M,
  typename... Variants
>
void MultiConsumerPort<T, M, Variants...>::disconnect(const Port::Ptr& ptr) noexcept
{
  connection_manager_.disconnect(shared_from_this(), ptr);
  multi_queue_.removeQueue(ptr);
}

template<
  typename T,
  GetMode M,
  typename... Variants
>
bool MultiConsumerPort<T, M, Variants...>::isConnected() const
{
  return connection_manager_.getNumConnections() > 0;
}

template<
  typename T,
  GetMode M,
  typename... Variants
>
PortStatus MultiConsumerPort<T, M, Variants...>::getStatus() const
{
  return {
    connection_manager_.getNumConnections(),
    num_transactions_
  };
}

template<
  typename T,
  GetMode M,
  typename... Variants
>
std::optional<std::vector<T>> MultiConsumerPort<T, M, Variants...>::getNext()
{
  try
  {
    std::vector<T> items;
    queue_getter_.get(multi_queue_, items);

    ++num_transactions_;

    return items;
  }
  catch (const TerminatedException&)
  { return std::nullopt; }
}

template<
  typename T,
  GetMode M,
  typename... Variants
>
std::vector<T> MultiConsumerPort<T, M, Variants...>::get()
{
  std::vector<T> items;
  queue_getter_.get(multi_queue_, items);

  ++num_transactions_;

  return items;
}

template<
  typename T,
  GetMode M,
  typename... Variants
>
bool MultiConsumerPort<T, M, Variants...>::hasNext() const
{
  return queue_getter_.hasNext(multi_queue_);
}

template<
  typename T,
  GetMode M,
  typename... Variants
>
MultiConsumerPort<T, M, Variants...>::operator bool() const
{
  return !multi_queue_.isTerminated();
}

template<
  typename T,
  GetMode M,
  typename... Variants
>
void MultiConsumerPort<T, M, Variants...>::clear()
{
  multi_queue_.clear();
}

template<
  typename T,
  GetMode M,
  typename... Variants
>
void MultiConsumerPort<T, M, Variants...>::deactivate()
{
  multi_queue_.terminate();
}
}
