// Copyright 2019, Forsvarets forskningsinstitutt. All rights reserved.
#pragma once

#include "superflow/proxel.h"

#include <functional>

namespace flow
{
/// \brief Function for creating a new Proxel
/// \param name[in] The name of the Proxel
/// \param properties[in] configuration properties for the Proxel
template<typename PropertyList>
using Factory = std::function<Proxel::Ptr(
  const PropertyList& properties
)>;
}
