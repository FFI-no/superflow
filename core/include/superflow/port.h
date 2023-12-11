// Copyright 2019, Forsvarets forskningsinstitutt. All rights reserved.
#pragma once

#include "superflow/port_status.h"

#include <memory>

namespace flow
{
/// \brief Interface for interconnection between two entities exchanging data.
class Port
{
public:
  using Ptr = std::shared_ptr<Port>;

  virtual ~Port() = default;

  /**
   * \brief Attempts to connect ports. Does nothing if already connected
   * to ptr. Throws if already connected and multiple connections is
   * unsupported.
   * \throw Exception if connection fails
   */
  virtual void connect(const Port::Ptr& ptr) = 0;

  /**
   * \brief disconnects port(s) if connected
   * otherwise does nothing
   */
  virtual void disconnect() noexcept = 0;

  /**
   * \brief Disconnects port if connected
   * otherwise does nothing
   */
  virtual void disconnect(const Port::Ptr& ptr) noexcept = 0;

  [[nodiscard]] virtual bool isConnected() const = 0;

  [[nodiscard]] virtual PortStatus getStatus() const = 0;
};
}