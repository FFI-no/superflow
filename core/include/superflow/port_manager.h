// Copyright 2019, Forsvarets forskningsinstitutt. All rights reserved.
#pragma once

#include "superflow/port.h"

#include <map>
#include <string>

namespace flow
{
/// \brief Container for managing Ports within a Proxel
/// \see Port, Proxel
class PortManager
{
public:
  /// \brief A map mapping port names to port pointers
  using PortMap = std::map<std::string, Port::Ptr>;

  /// \brief Create a new PortManager to manage a PortMap
  /// \param ports an existing PortMap
  explicit PortManager(const PortMap& ports);

  /// \brief Create a PortManager and let the manager own all Ports
  /// \param ports an existing PortMap
  explicit PortManager(PortMap&& ports);

  ~PortManager();

  PortManager() = default;

  PortManager(const PortManager&) = default;

  PortManager(PortManager&&) = default;

  PortManager& operator=(const PortManager&) = default;

  PortManager& operator=(PortManager&&) = default;

  /// \brief Get the port with the given name
  /// \param name The name of the Port
  /// \return A pointer to the Port
  [[nodiscard]] const Port::Ptr& get(const std::string& name) const;

  /// \brief Request access to the PortMap managed by the PortManager.
  /// \return The PortMap
  [[nodiscard]] const PortMap& getPorts() const;

  /// \brief Get the status of all Ports
  /// \return A map from port name to port status
  /// \see PortStatus
  [[nodiscard]] std::map<std::string, PortStatus> getStatus() const;

private:
  PortMap ports_;
};
}
