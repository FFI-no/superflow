// Copyright 2019, Forsvarets forskningsinstitutt. All rights reserved.
#pragma once

#include "superflow/port.h"

#include <stdexcept>

namespace flow
{
template<typename T>
class ConnectablePort :
    public Port,
    public std::enable_shared_from_this<ConnectablePort<T>>
{
public:
  using Ptr = std::shared_ptr<ConnectablePort>;

  std::shared_ptr<ConnectablePort> connection_;
  size_t num_connections = PortStatus::undefined;
  size_t num_transactions = PortStatus::undefined;
  bool did_get_disconnect = false;

  void connect(const Port::Ptr& ptr) override
  {
    auto connection = std::dynamic_pointer_cast<ConnectablePort>(ptr);

    if (connection == nullptr)
    {
      throw std::invalid_argument("Mismatch between port types");
    }

    connection_ = std::move(connection);
    connection_->connection_ = this->shared_from_this();
  }

  void disconnect() noexcept override
  {
    did_get_disconnect = true;

    if (connection_ == nullptr)
    {
      return;
    }

    connection_->connection_ = nullptr;
    connection_ = nullptr;
  }

  void disconnect(const Port::Ptr& ptr) noexcept override
  {
    if (connection_ != ptr)
    {
      return;
    }

    disconnect();
  }

  bool isConnected() const override
  {
    return connection_ != nullptr;
  }

  PortStatus getStatus() const override
  {
    return {
        num_connections,
        num_transactions
    };
  }
};
}
