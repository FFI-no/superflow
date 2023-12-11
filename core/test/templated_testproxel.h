// Copyright 2019, Forsvarets forskningsinstitutt. All rights reserved.
#pragma once

#include "superflow/buffered_consumer_port.h"
#include "superflow/policy.h"
#include "superflow/port_manager.h"
#include "superflow/proxel.h"
#include "superflow/producer_port.h"
#include "superflow/value.h"

#include <map>

namespace flow
{
template<typename T>
class TemplatedProxel : public Proxel
{
public:
  explicit TemplatedProxel(T init_value = {})
      : value_{std::move(init_value)}
      , out_port_{std::make_shared<OutPort>()}
      , in_port_{std::make_shared<InPort>()}
  {
    registerPorts(
        {
            {"outport", out_port_},
            {"inport",  in_port_}
        });
  }

  void start() override
  { out_port_->send(value_); }

  void stop() noexcept override
  {}

  T getValue()
  {
    (*in_port_) >> value_;
    return value_;
  }

  T getStoredValue() const
  { return value_; }

  template<typename PropertyList>
  static flow::Proxel::Ptr create(
    const PropertyList& properties
  )
  {
    return std::make_shared<TemplatedProxel<T>>(
        value<T>(properties, "init_value")
    );
  }

private:
  using InPort = BufferedConsumerPort<T, ConnectPolicy::Single>;
  using OutPort = ProducerPort<T>;

  T value_;

  typename OutPort::Ptr out_port_;
  typename InPort::Ptr in_port_;
};
}
