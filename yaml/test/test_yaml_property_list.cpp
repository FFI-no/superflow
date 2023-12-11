// Copyright 2023, Forsvarets forskningsinstitutt. All rights reserved.
#include "gtest/gtest.h"
#include "superflow/loader/proxel_library.h"
#include "superflow/yaml/yaml_property_list.h"

#include "boost/dll/runtime_symbol_info.hpp"    // for program_location()
#include <iostream>


namespace dll = boost::dll;

TEST(SuperflowYaml, can_load_load_test_libary)
{
  ASSERT_NO_FATAL_FAILURE(flow::load::ProxelLibrary library{dll::program_location()});
}

TEST(SuperflowYaml, can_load_adapter_name)
{
  const flow::load::ProxelLibrary library{dll::program_location()};

  ASSERT_NO_THROW(
    std::ignore = library.loadFactories<flow::yaml::YAMLPropertyList>()
  );
}

TEST(SuperflowYaml, can_load_factories_and_create_proxel)
{
  const flow::load::ProxelLibrary library{dll::program_location()};

  const auto factories = library.loadFactories<flow::yaml::YAMLPropertyList>();

  ASSERT_FALSE(factories.empty());

  const auto& proxel_factory = factories.get("YamlTestProxel");

  constexpr int the_number{42};
  const auto load_yaml_properties = [the_number]
  {
    YAML::Node node;
    node["int"] = the_number;
    return flow::yaml::YAMLPropertyList(node);
  };

  const auto properties = load_yaml_properties();
  const auto proxel = std::invoke(proxel_factory, properties);

  EXPECT_EQ(flow::ProxelStatus::State::AwaitingInput, proxel->getStatus().state);
  ASSERT_EQ(std::to_string(the_number), proxel->getStatus().info);
}
