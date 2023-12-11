// Copyright 2019, Forsvarets forskningsinstitutt. All rights reserved.
#pragma once

#include "superflow/connection_spec.h"
#include "superflow/factory_map.h"
#include "superflow/graph.h"
#include "superflow/proxel_config.h"

namespace flow
{
template<typename PropertyList>
inline std::map<std::string, Proxel::Ptr> createProxelsFromConfig(
    const FactoryMap<PropertyList>& factory_map,
    const std::vector<ProxelConfig<PropertyList>>& proxel_configurations)
{
  std::map<std::string, Proxel::Ptr> proxels{};

  for (const auto& config : proxel_configurations)
  {
    if (proxels.count(config.id) > 0)
    { throw std::invalid_argument("Proxel with id '"+ config.id +"' is defined more than once."); }

    const auto& factory = factory_map.get(config.type);
    try
    {
      proxels.emplace(config.id, factory(config.properties));
    }
    catch(const std::exception& e)
    {
      throw std::runtime_error("Failed to create proxel '" + config.id + "': " + e.what());
    }
  }

  return proxels;
}

template<typename PropertyList>
inline Graph createGraph(
    const FactoryMap<PropertyList>& factory_map,
    const std::vector<ProxelConfig<PropertyList>>& proxel_configurations,
    const std::vector<ConnectionSpec>& connections
)
{
  Graph graph{
      createProxelsFromConfig(factory_map, proxel_configurations)
  };

  for (const auto& connection : connections)
  { graph.connect(connection.lhs_name, connection.lhs_port, connection.rhs_name, connection.rhs_port); }

  return graph;
}
}
