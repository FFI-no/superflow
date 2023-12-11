// Copyright 2019, Forsvarets forskningsinstitutt. All rights reserved.
#pragma once

#include <memory>
#include <string>

namespace flow
{
template<typename PropertyList>
struct ProxelConfig
{
  std::string id;
  std::string type;
  PropertyList properties;
};
}
