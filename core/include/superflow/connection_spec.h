// Copyright 2019, Forsvarets forskningsinstitutt. All rights reserved.
#pragma once

#include <string>

namespace flow
{
struct ConnectionSpec
{
  std::string lhs_name;
  std::string lhs_port;
  std::string rhs_name;
  std::string rhs_port;
};
}
