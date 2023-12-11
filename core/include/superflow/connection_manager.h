// Copyright 2019, Forsvarets forskningsinstitutt. All rights reserved.
#pragma once

#include "superflow/policy.h"
#include "superflow/port.h"

#include <stdexcept>
#include <unordered_set>

namespace flow
{
/// \brief A utility class for handling connections between
/// Ports, while also encforcing ConnectPolicy.
///
/// A ConnectionManager is typically owned by a Port to keep
/// track of its connections.
/// The class focuses on connections only, and is agnostic to communication.
/// \tparam P Supported ConnectPolicys are Single and Multi.
template<ConnectPolicy P>
class ConnectionManager
{
public:
  /// \brief Register a new connection and call other->connect(owner).
  /// \param owner Typically the owner of the ConnectionManager
  /// \param other The external Port to connect to.
  void connect(
      const Port::Ptr& owner,
      const Port::Ptr& other
  );

  /// \brief Disconnect from all registered connections.
  /// \param owner Typically the owner of the ConnectionManager
  void disconnect(
      const Port::Ptr& owner
  ) noexcept;

  /// \brief Disconnect a specific Port
  /// \param owner Typically the owner of the ConnectionManager
  /// \param other The external Port to disconnect from.
  void disconnect(
      const Port::Ptr& owner,
      const Port::Ptr& other
  ) noexcept;

  /// \brief Get the number of connection pairs managed by the ConnectionManager
  size_t getNumConnections() const;

  /// \brief True if the ConnectionManager manages at least one connection.
  bool isConnected() const;

private:
  std::unordered_set<Port::Ptr> connections_;

  bool hasPort(const Port::Ptr& ptr) const;

  Port::Ptr front() const;
};

template<>
inline bool ConnectionManager<ConnectPolicy::Multi>::hasPort(
    const Port::Ptr& ptr
) const
{
  return connections_.find(ptr) != connections_.end();
}

template<>
inline Port::Ptr ConnectionManager<ConnectPolicy::Single>::front() const
{
  return connections_.empty()
         ? nullptr
         : *connections_.cbegin();
}

template<>
inline void ConnectionManager<ConnectPolicy::Multi>::connect(
    const Port::Ptr& owner,
    const Port::Ptr& other
)
{
  if (hasPort(other))
  { return; }

  connections_.insert(other);

  try
  {
    other->connect(owner);
  }
  catch (...)
  {
    connections_.erase(other);
    std::rethrow_exception(std::current_exception());
  }
}

template<>
inline void ConnectionManager<ConnectPolicy::Single>::connect(
    const Port::Ptr& owner,
    const Port::Ptr& other
)
{
  if (other == front())
  { return; }

  if (!connections_.empty())
  { throw std::invalid_argument("Attempted connecting multiple ports to Single-port"); }

  connections_.insert(other);

  try
  {
    other->connect(owner);
  }
  catch (...)
  {
    connections_.clear();
    std::rethrow_exception(std::current_exception());
  }
}

template<ConnectPolicy P>
inline void ConnectionManager<P>::disconnect(
    const Port::Ptr& owner
) noexcept
{
  std::unordered_set<Port::Ptr> old_connections;
  std::swap(connections_, old_connections);

  for (const auto& connection : old_connections)
  {
    connection->disconnect(owner);
  }
}

template<>
inline void ConnectionManager<ConnectPolicy::Multi>::disconnect(
    const Port::Ptr& owner,
    const Port::Ptr& other
) noexcept
{
  auto it = connections_.find(other);

  if (it == connections_.end())
  {
    // already disconnected
    return;
  }

  connections_.erase(it);
  other->disconnect(owner);
}

template<>
inline void ConnectionManager<ConnectPolicy::Single>::disconnect(
    const Port::Ptr& owner,
    const Port::Ptr& other
) noexcept
{
  if (other != front())
  {
    // already disconnected
    return;
  }

  connections_.clear();
  other->disconnect(owner);
}

template<ConnectPolicy P>
inline size_t ConnectionManager<P>::getNumConnections() const
{
  return connections_.size();
}

template<ConnectPolicy P>
inline bool ConnectionManager<P>::isConnected() const
{
  return getNumConnections() > 0;
}
}
