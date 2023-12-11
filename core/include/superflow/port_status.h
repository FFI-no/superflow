// Copyright 2019, Forsvarets forskningsinstitutt. All rights reserved.
#pragma once

#include <cstddef>
#include <limits>

namespace flow
{
/// \brief A container for statistics and status for a Port
/// \see Port
struct PortStatus
{
  inline static constexpr size_t undefined = std::numeric_limits<size_t>::max();

  size_t num_connections;  ///< Number of connections to the Port
  size_t num_transactions; ///< Number of transactions passed through the Port
};
}
