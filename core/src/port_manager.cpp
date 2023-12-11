// Copyright 2019, Forsvarets forskningsinstitutt. All rights reserved.
#include "superflow/port_manager.h"

#include <stdexcept>

namespace flow
{
PortManager::PortManager(const PortMap& ports)
    : ports_{ports}
{}

PortManager::PortManager(PortMap&& ports)
    : ports_{std::move(ports)}
{}

PortManager::~PortManager()
{
  for (auto& kv : ports_)
  {
    Port::Ptr& port = kv.second;

    if (port == nullptr)
    {
      continue;
    }

    port->disconnect();
  }
}

const Port::Ptr& PortManager::get(const std::string& name) const
{
  try
  { return ports_.at(name); }
  catch (...)
  { throw std::invalid_argument(std::string("port '" + name + "' does not exist")); }
}

const PortManager::PortMap& PortManager::getPorts() const
{
  return ports_;
}

std::map<std::string, PortStatus> PortManager::getStatus() const
{
  std::map<std::string, PortStatus> statuses;

  for (const auto& [port_name, port] : ports_)
  {
    if (port == nullptr)
    { continue; }

    statuses[port_name] = port->getStatus();
  }

  return statuses;
}
}
