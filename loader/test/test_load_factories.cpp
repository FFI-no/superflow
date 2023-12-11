// Copyright 2019, Forsvarets forskningsinstitutt. All rights reserved.
#include "testing/dummy_value_adapter.h"

#include "superflow/loader/load_factories.h"
#include "proxels/lib_path.h"
#include "proxels/fauxade.h"

#include "gtest/gtest.h"

using namespace flow;

TEST(LoadFactories, loadFactories_from_vector_of_libraries)
{
  const std::vector<load::ProxelLibrary> libraries{
    {flow::test::local::lib_dir, "proxels"}
  };

  flow::FactoryMap<flow::test::DummyValueAdapter> factories;

  ASSERT_NO_FATAL_FAILURE(
    factories = load::loadFactories<flow::test::DummyValueAdapter>(libraries)
  );

  ASSERT_FALSE(factories.empty());
}
