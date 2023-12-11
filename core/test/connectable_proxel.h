// Copyright 2019, Forsvarets forskningsinstitutt. All rights reserved.
#pragma once

#include "superflow/proxel.h"

namespace flow
{
class ConnectableProxel : public Proxel
{
public:
  ConnectableProxel(
      const Port::Ptr& in_port,
      const Port::Ptr& out_port
  )
      : in_port_{in_port}
      , out_port_{out_port}
  {
    registerPorts(
        {
            {"inport",  in_port_},
            {"outport", out_port_}
        }
    );
  }

  void start() override
  {};

  void stop() noexcept override
  {};

  Port::Ptr in_port_;
  Port::Ptr out_port_;
};
}
