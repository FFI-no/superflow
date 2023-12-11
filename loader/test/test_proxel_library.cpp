// Copyright 2019, Forsvarets forskningsinstitutt. All rights reserved.
#include "testing/dummy_value_adapter.h"

#include "superflow/loader/proxel_library.h"
#include "proxels/lib_path.h"
#include "proxels/fauxade.h"

#include "gtest/gtest.h"

using namespace flow;

TEST(ProxelLibrary, load_libary_by_library_name)
{
  ASSERT_NO_FATAL_FAILURE(
    load::ProxelLibrary(flow::test::local::lib_dir, "proxels")
  );
}

TEST(ProxelLibrary, load_libary_by_full_path)
{
  ASSERT_NO_FATAL_FAILURE(load::ProxelLibrary(boost::dll::fs::path{flow::test::local::lib_path}));
}

TEST(ProxelLibrary, nonexisting_path)
{
  ASSERT_THROW(
    load::ProxelLibrary(boost::dll::fs::path{flow::test::local::lib_path} / "non-existing path"),
    std::invalid_argument
  );
}

TEST(ProxelLibrary, empty_path)
{
  ASSERT_THROW(
    load::ProxelLibrary(boost::dll::fs::path{}),
    std::invalid_argument
  );
}

TEST(ProxelLibrary, loadFactories_from_library)
{
  const load::ProxelLibrary library{flow::test::local::lib_dir, "proxels"};

  ASSERT_NO_THROW(
    std::ignore = library.loadFactories<flow::test::DummyValueAdapter>()
  );
}

TEST(ProxelLibrary, nonexisting_proxel_name)
{
  const load::ProxelLibrary library{flow::test::local::lib_dir, "proxels"};
  const auto factories = library.loadFactories<flow::test::DummyValueAdapter>();

  ASSERT_FALSE(factories.empty());

  EXPECT_THROW(
    factories.get("Non-existing proxel name"),
    std::invalid_argument
  );
}

TEST(ProxelLibrary, can_create_proxel_from_factory)
{
  const load::ProxelLibrary library{flow::test::local::lib_dir, "proxels"};
  const auto factories = library.loadFactories<flow::test::DummyValueAdapter>();

  ASSERT_FALSE(factories.empty());

  const flow::test::DummyValueAdapter properties;
  const auto& dummy_factory = factories.get("Dummy");
  const auto dummy_proxel = std::invoke(dummy_factory, properties);

  EXPECT_EQ(ProxelStatus::State::Paused, dummy_proxel->getStatus().state);
}

TEST(ProxelLibrary, can_create_proxel_with_value)
{
  const load::ProxelLibrary library{flow::test::local::lib_dir, "proxels"};
  const auto factories = library.loadFactories<flow::test::DummyValueAdapter>();

  ASSERT_FALSE(factories.empty());

  const flow::test::DummyValueAdapter properties;
  const auto yummy_proxel = std::invoke(factories.get("Yummy"), properties);

  EXPECT_EQ(ProxelStatus::State::AwaitingInput, yummy_proxel->getStatus().state);
  ASSERT_EQ(
    std::to_string(properties.int_value),
    yummy_proxel->getStatus().info
  );
}


TEST(ProxelLibrary, load_factories_with_args)
{
  const load::ProxelLibrary library{flow::test::local::lib_dir, "proxels"};
  const auto fauxade_ptr = std::make_shared<flow::test::Fauxade>(42);
  ASSERT_EQ(42, fauxade_ptr->val);

  const auto factories = library.loadFactories<flow::test::DummyValueAdapter>(
    fauxade_ptr
  );

  ASSERT_FALSE(factories.empty());

  const flow::test::DummyValueAdapter properties;
  const auto& mummy_factory = factories.get("Mummy");
  const auto mummy_proxel = std::invoke(mummy_factory, properties);

  ASSERT_EQ("42", mummy_proxel->getStatus().info);
}

TEST(ProxelLibrary, ProxelWorks)
{
  const load::ProxelLibrary library{flow::test::local::lib_dir, "proxels"};
  const auto factories = library.loadFactories<flow::test::DummyValueAdapter>();

  ASSERT_FALSE(factories.empty());

  const flow::test::DummyValueAdapter properties;
  const auto& dummy_factory = factories.get("Dummy");
  const auto dummy_proxel = std::invoke(dummy_factory, properties);

  EXPECT_EQ(ProxelStatus::State::Paused, dummy_proxel->getStatus().state);
  dummy_proxel->start();
  EXPECT_EQ(ProxelStatus::State::Running, dummy_proxel->getStatus().state);
  dummy_proxel->stop();
  EXPECT_EQ(ProxelStatus::State::Unavailable, dummy_proxel->getStatus().state);
}
