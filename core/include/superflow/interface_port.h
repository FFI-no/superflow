// Copyright 2019, Forsvarets forskningsinstitutt. All rights reserved.
#pragma once
#include "superflow/connection_manager.h"
#include "superflow/port.h"

#include <atomic>
#include <ciso646>
#include <memory>

namespace flow
{
template<typename Interface>
struct InterfacePort
{
  class Host :
    public Port,
    public std::enable_shared_from_this<Port>
  {
  public:
    using Ptr = std::shared_ptr<Host>;

    explicit Host(
      Interface& handle
    );

    ~Host() override = default;

    void connect(const Port::Ptr& ptr) override;

    void disconnect() noexcept override;

    void disconnect(const Port::Ptr& ptr) noexcept override;

    bool isConnected() const override;

    PortStatus getStatus() const override;

    const Interface& get() const;

    Interface& get();

  private:
    mutable std::atomic<size_t> num_transactions_ = 0;
    Interface& handle_;
    ConnectionManager<ConnectPolicy::Multi> connection_manager_;
  };

  class Client :
    public Port,
    public std::enable_shared_from_this<Port>
  {
  public:
    using Ptr = std::shared_ptr<Client>;

    ~Client() override = default;

    void connect(const Port::Ptr& ptr) override;

    void disconnect() noexcept override;

    void disconnect(const Port::Ptr& ptr) noexcept override;

    bool isConnected() const override;

    PortStatus getStatus() const override;

    const Interface& get() const;

    Interface& get();

  private:
    mutable std::atomic<size_t> num_transactions_ = 0;
    std::shared_ptr<Host> host_;
  };
};

// ----- Implementation -----
template<typename Interface>
InterfacePort<Interface>::Host::Host(Interface& handle)
  : handle_{handle}
{}

template<typename Interface>
void InterfacePort<Interface>::Host::connect(const Port::Ptr& ptr)
{
  connection_manager_.connect(shared_from_this(), ptr);
}

template<typename Interface>
void InterfacePort<Interface>::Host::disconnect() noexcept
{
  connection_manager_.disconnect(shared_from_this());
}

template<typename Interface>
void InterfacePort<Interface>::Host::disconnect(const Port::Ptr& ptr) noexcept
{
  connection_manager_.disconnect(shared_from_this(), ptr);
}

template<typename Interface>
bool InterfacePort<Interface>::Host::isConnected() const
{
  return connection_manager_.isConnected();
}

template<typename Interface>
PortStatus InterfacePort<Interface>::Host::getStatus() const
{
  return {
    connection_manager_.getNumConnections(),
    num_transactions_
  };
}

template<typename Interface>
const Interface& InterfacePort<Interface>::Host::get() const
{
  ++num_transactions_;
  if (not isConnected())
  { throw std::runtime_error("InterfacePort::Host has no connection."); }

  return handle_;
}

template<typename Interface>
Interface& InterfacePort<Interface>::Host::get()
{
  ++num_transactions_;
  if (not isConnected())
  { throw std::runtime_error("InterfacePort::Host has no connection."); }
  return handle_;
}

template<typename Interface>
void InterfacePort<Interface>::Client::connect(const Port::Ptr& ptr)
{
  if (host_ == ptr)
  { return; }

  const auto host = std::dynamic_pointer_cast<Host>(ptr);

  if (host == nullptr)
  { throw std::invalid_argument{std::string("Type mismatch when connecting ports")}; }

  if (isConnected())
  { disconnect(); }

  host_ = host;
  ptr->connect(shared_from_this());
}

template<typename Interface>
void InterfacePort<Interface>::Client::disconnect() noexcept
{
  if (host_ == nullptr)
  { return; }

  const auto host = std::move(host_);
  host->disconnect();
}

template<typename Interface>
void InterfacePort<Interface>::Client::disconnect(const Port::Ptr& ptr) noexcept
{
  if (host_ != ptr)
  { return; }

  disconnect();
}

template<typename Interface>
bool InterfacePort<Interface>::Client::isConnected() const
{
  return host_ != nullptr;
}

template<typename Interface>
PortStatus InterfacePort<Interface>::Client::getStatus() const
{
  return {
    static_cast<size_t>(isConnected()),
    num_transactions_
  };
}

template<typename Interface>
const Interface& InterfacePort<Interface>::Client::get() const
{
  ++num_transactions_;

  if (not isConnected())
  { throw std::runtime_error("InterfacePort::Client has no connection."); }

  return host_->get();
}

template<typename Interface>
Interface& InterfacePort<Interface>::Client::get()
{
  ++num_transactions_;
  if (not isConnected())
  { throw std::runtime_error("InterfacePort::Client has no connection."); }

  return host_->get();
}
}
