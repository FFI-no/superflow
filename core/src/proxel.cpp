// Copyright 2019, Forsvarets forskningsinstitutt. All rights reserved.
#include "superflow/proxel.h"

namespace flow
{
const Port::Ptr& Proxel::getPort(const std::string& name) const
{
  return port_manager_.get(name);
}

const PortManager::PortMap& Proxel::getPorts() const
{
  return port_manager_.getPorts();
}

ProxelStatus Proxel::getStatus() const
{
  return {
      getState(),
      getStatusInfo(),
      port_manager_.getStatus()
  };
}

void Proxel::setState(const State state) const
{
  state_ = state;
}

void Proxel::setStatusInfo(const std::string& status_info) const
{
  status_info_.store(status_info);
}

Proxel::State Proxel::getState() const
{
  return state_;
}

std::string Proxel::getStatusInfo() const
{
  return status_info_.load();
}

void Proxel::registerPorts(PortManager::PortMap&& ports)
{
  port_manager_ = PortManager{std::move(ports)};
}
}
