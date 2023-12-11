// Copyright 2019, Forsvarets forskningsinstitutt. All rights reserved.
#pragma once

#include "superflow/port.h"
#include "superflow/port_manager.h"
#include "superflow/proxel_status.h"
#include "superflow/utils/mutexed.h"

#include <atomic>
#include <memory>

namespace flow
{
/// \brief Abstract class for Processing Element
///
/// A Proxel, short for processing element, is an isolated "black box"
/// responsible for doing some kind of data manipulation.
/// Proxel is in fact just an interface defining a handful of required methods.
/// Besides that, a class extending the Proxel interface may do whatever
/// the developer decides. The recommended way to create a Proxel is to develop
/// a processing algorithms independently as an external library,
/// and then embed the algorithm into Superflow by wrapping it in a Proxel.
class Proxel
{
public:
  using Ptr = std::shared_ptr<Proxel>;
  using ConstPtr = std::shared_ptr<const Proxel>;

  virtual ~Proxel() = default;

  /// \brief Method is expected to prepare the Proxel for processing,
  /// and make it listen to it's input ports.
  virtual void start() = 0;

  /// \brief Method is expected to make the Proxel stop processing and
  /// thereby stop producing outputs. If the Proxel is designed to have
  /// start() called by an external thread, stop() is expected to make that
  /// thread return.
  virtual void stop() noexcept = 0;

  /// \brief Request a pointer to a Proxel's port by a given name.
  /// \param name The unique name of the port
  /// \return
  const Port::Ptr& getPort(const std::string& name) const;

  const PortManager::PortMap& getPorts() const;

  /// \brief Request the current status of the Proxel.
  /// \return
  ProxelStatus getStatus() const;

protected:
  using State = ProxelStatus::State;

  void setState(State state) const;

  void setStatusInfo(const std::string& status_info) const;

  void registerPorts(PortManager::PortMap&& ports);

private:
  mutable std::atomic<State> state_ = State::Undefined;
  mutable Mutexed<std::string> status_info_;
  PortManager port_manager_;

  State getState() const;

  std::string getStatusInfo() const;
};
}
