// Copyright 2019, Forsvarets forskningsinstitutt. All rights reserved.
#pragma once

#include "superflow/connection_manager.h"
#include "superflow/mapped_asset_manager.h"
#include "superflow/port.h"
#include "superflow/responder_port.h"

#include <ciso646>
#include <future>
#include <vector>

namespace flow
{
template<typename>
class MultiRequesterPort;

/// \brief A MasterPort able to simultaneously request data from several SlavePort
/// \tparam ReturnValue The type av data to request
/// \tparam Args arguments necessary for the SlavePort to respond
template<typename ReturnValue, typename ...Args>
class MultiRequesterPort<ReturnValue(Args...)> final :
    public Port,
    public std::enable_shared_from_this<Port>
{
public:
  using Ptr = std::shared_ptr<MultiRequesterPort>;
  void connect(const Port::Ptr& ptr) override;

  void disconnect() noexcept override;

  void disconnect(const Port::Ptr& ptr) noexcept override;

  PortStatus getStatus() const override;

  bool isConnected() const override;

  /// \brief Request new data from all SlavePort
  /// \param args arguments necessary for the SlavePort to respond
  /// \return Response from all SlavePort
  template<typename RV = ReturnValue>
  typename std::enable_if_t<not std::is_same_v<RV, void>, std::vector<RV>>
  request(Args... args);

  template<typename RV = ReturnValue>
  typename std::enable_if_t<std::is_same_v<RV, void>> // default er jo void. Flaks! (litt mindre lesbart, dog)
  request(Args... args);


  std::vector<std::future<ReturnValue>> requestAsync(Args... args);

private:
  using Connection = ResponderPort<ReturnValue(Args...)>;
  using ConnectionPtr = std::shared_ptr<Connection>;

  size_t num_transactions_ = 0;
  ConnectionManager<ConnectPolicy::Multi> connection_manager_;
  MappedAssetManager<Port::Ptr, ConnectionPtr> slaves_;
};

// ----- Implementation -----
template<typename ReturnValue, typename ...Args>
void MultiRequesterPort<ReturnValue(Args...)>::connect(const Port::Ptr& ptr)
{
  auto slave = std::dynamic_pointer_cast<ResponderPort<ReturnValue(Args...)>>(ptr);

  if (slave == nullptr)
  { throw std::invalid_argument{std::string("Type mismatch when connecting ports")}; }

  connection_manager_.connect(shared_from_this(), ptr);
  slaves_.put(ptr, slave);
}

template<typename ReturnValue, typename ...Args>
void MultiRequesterPort<ReturnValue(Args...)>::disconnect() noexcept
{
  connection_manager_.disconnect(shared_from_this());
  slaves_.clear();
}

template<typename ReturnValue, typename ...Args>
void MultiRequesterPort<ReturnValue(Args...)>::disconnect(const Port::Ptr& ptr) noexcept
{
  connection_manager_.disconnect(shared_from_this(), ptr);
  slaves_.erase(ptr);
}

template<typename ReturnValue, typename ...Args>
PortStatus MultiRequesterPort<ReturnValue(Args...)>::getStatus() const
{
  return {
      connection_manager_.getNumConnections(),
      num_transactions_
  };
}

template<typename ReturnValue, typename ...Args>
bool MultiRequesterPort<ReturnValue(Args...)>::isConnected() const
{
  return connection_manager_.isConnected();
}

template<typename ReturnValue, typename ...Args>
template<typename RV>
typename std::enable_if_t<not std::is_same_v<RV, void>, std::vector<RV>>
MultiRequesterPort<ReturnValue(Args...)>::request(Args... args)
{
  const auto slaves = slaves_.getAll();

  std::vector<ReturnValue> responses;
  responses.reserve(slaves.size());

  ++num_transactions_;

  for (const auto& slave : slaves)
  {
    responses.push_back(slave->respond(args...));
  }

  return responses;
}

template<typename ReturnValue, typename ...Args>
template<typename RV>
typename std::enable_if_t<std::is_same_v<RV, void>, void>
MultiRequesterPort<ReturnValue(Args...)>::request(Args... args)
{
  const auto slaves = slaves_.getAll();

  for (const auto& slave : slaves)
  {
    slave->respond(args...);
  }

  ++num_transactions_;
}

template<typename ReturnValue, typename ...Args>
inline std::vector<std::future<ReturnValue>> MultiRequesterPort<ReturnValue(Args...)>::requestAsync(Args... args)
{
  const auto slaves = slaves_.getAll();

  std::vector<std::future<ReturnValue>> responses;
  responses.reserve(slaves.size());

  ++num_transactions_;

  for (const auto& slave : slaves)
  {
    responses.push_back(
        std::async(
            std::launch::async,
            [slave, args...]() { return slave->respond(args...); }
        )
    );
  }

  return responses;
}
}
