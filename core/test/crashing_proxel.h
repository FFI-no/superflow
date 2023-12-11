// Copyright 2019, Forsvarets forskningsinstitutt. All rights reserved.
#pragma once

#include "superflow/proxel.h"

namespace flow
{
class CrashingProxel : public Proxel
{
public:
  void start() override
  {
    throw std::runtime_error("This proxel has crashed");
  }

  void stop() noexcept override
  {}
};
}
