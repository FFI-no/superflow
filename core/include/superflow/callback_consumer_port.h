// Copyright 2019, Forsvarets forskningsinstitutt. All rights reserved.
#pragma once

#include "superflow/connection_manager.h"
#include "superflow/consumer_port.h"
#include "superflow/policy.h"

#include <functional>

namespace flow
{
/// \brief This port calls a function every time data is received.
/// \tparam T The type of data to be exchanged between ports.
/// \tparam P ConnectPolicy
/// \tparam Variants... Optionally supported input variant types. \see ConsumerPort
template<
  typename T,
  ConnectPolicy P = ConnectPolicy::Single,
  typename... Variants
>
class CallbackConsumerPort :
    public ConsumerPort<T, Variants...>,
    public std::enable_shared_from_this<Port>
{
public:
  using Ptr = std::shared_ptr<CallbackConsumerPort>;
  using Callback = std::function<void(const T&)>;

  explicit CallbackConsumerPort(const Callback&);

  void receive(const T&, const Port::Ptr&) override;

  void connect(const Port::Ptr& ptr) override;

  void disconnect() noexcept override;

  void disconnect(const Port::Ptr& ptr) noexcept override;

  bool isConnected() const override;

  PortStatus getStatus() const override;

private:
  size_t num_transactions_ = 0;

  Callback callback_;
  ConnectionManager<P> connection_manager_;
};

// --- Implementations --- //
template<
  typename T,
  ConnectPolicy P,
  typename... Variants
>
CallbackConsumerPort<T, P, Variants...>::CallbackConsumerPort(const Callback& callback)
    : callback_{callback}
{}

template<
  typename T,
  ConnectPolicy P,
  typename... Variants
>
inline void CallbackConsumerPort<T, P, Variants...>::receive(const T& t, const Port::Ptr&)
{
  callback_(t);
  ++num_transactions_;
}

template<
  typename T,
  ConnectPolicy P,
  typename... Variants
>
void CallbackConsumerPort<T, P, Variants...>::connect(const Port::Ptr& ptr)
{
  connection_manager_.connect(shared_from_this(), ptr);
}

template<
  typename T,
  ConnectPolicy P,
  typename... Variants
>
void CallbackConsumerPort<T, P, Variants...>::disconnect() noexcept
{
  connection_manager_.disconnect(shared_from_this());
}

template<
  typename T,
  ConnectPolicy P,
  typename... Variants
>
void CallbackConsumerPort<T, P, Variants...>::disconnect(const Port::Ptr& ptr) noexcept
{
  connection_manager_.disconnect(shared_from_this(), ptr);
}

template<
  typename T,
  ConnectPolicy P,
  typename... Variants
>
bool CallbackConsumerPort<T, P, Variants...>::isConnected() const
{
  return connection_manager_.isConnected();
}

template<
  typename T,
  ConnectPolicy P,
  typename... Variants
>
PortStatus CallbackConsumerPort<T, P, Variants...>::getStatus() const
{
  return {
      connection_manager_.getNumConnections(),
      num_transactions_
  };
}
}
