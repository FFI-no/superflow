// Copyright 2019, Forsvarets forskningsinstitutt. All rights reserved.
#pragma once

#include "superflow/port.h"

#include <unordered_set>

namespace flow
{
template<typename T>
class MultiConnectablePort :
    public Port,
    public std::enable_shared_from_this<MultiConnectablePort<T>>
{
public:
  using Ptr = std::shared_ptr<MultiConnectablePort>;
  std::unordered_set<std::shared_ptr<MultiConnectablePort>> connections_;
  bool did_get_disconnect = false;

  void connect(const Port::Ptr& ptr) override
  {
    auto connection = std::dynamic_pointer_cast<MultiConnectablePort>(ptr);

    if (connection == nullptr)
    {
      throw std::invalid_argument("Mismatch between port types");
    }

    connections_.insert(connection);
    connection->connections_.insert(this->shared_from_this());
  }

  void disconnect() noexcept override
  {
    did_get_disconnect = true;

    for (const auto& connection : connections_)
    {
      connection->connections_.erase(this->shared_from_this());
    }

    connections_.clear();
  }

  void disconnect(const Port::Ptr& ptr) noexcept override
  {
    did_get_disconnect = true;

    auto connection = std::dynamic_pointer_cast<MultiConnectablePort>(ptr);

    if (connection == nullptr)
    {
      return;
    }

    auto it = connections_.find(connection);

    if (it == connections_.end())
    {
      return;
    }

    (*it)->connections_.erase(this->shared_from_this());
    connections_.erase(it);
  }

  bool isConnected() const override
  {
    return !connections_.empty();
  }

  size_t getNumConnections() const
  {
    return connections_.size();
  }

  PortStatus getStatus() const override
  {
    return {
        getNumConnections(),
        0
    };
  }
};
}
