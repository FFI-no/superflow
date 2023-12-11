// Copyright 2019, Forsvarets forskningsinstitutt. All rights reserved.
#include "templated_testproxel.h"

#include "superflow/graph_factory.h"

#include "gtest/gtest.h"

using namespace flow;

class TestPropertyList
{

public:
  using Input = std::map<std::string, double>;

  explicit TestPropertyList(Input input)
      : input_{std::move(input)}
  {}

  bool hasKey(const std::string& key) const
  { return input_.count(key) > 0; }

  template<typename T>
  T convertValue(const std::string& key) const
  {
    if (!hasKey(key))
    { throw std::invalid_argument({"Could not find key '" + key + "' in PropertyList."}); }

    return static_cast<T>(input_.at(key));
  }

private:
  Input input_;
};

template<typename F>
Factory<TestPropertyList> getProxelFactory()
{
  return [](const TestPropertyList& properties)
  {
    return F::create(properties);
  };
}

TEST(GraphFactory, createGraph)
{
  using TestProxel = TemplatedProxel<double>;

  const FactoryMap<TestPropertyList> factories(
      {
          {"TemplatedProxel", getProxelFactory<TestProxel>()}
      });

  // for hver ting (sensor, publisher, proxel, ...)
  TestPropertyList::Input properties1 = {{"init_value", 42.0}};
  TestPropertyList::Input properties2 = {{"init_value", 21.0}};

  const auto config = std::vector<ProxelConfig<TestPropertyList>> {
      {"proxel1", "TemplatedProxel", TestPropertyList{std::move(properties1)}},
      {"proxel2", "TemplatedProxel", TestPropertyList{std::move(properties2)}}
  };

  std::vector<ConnectionSpec> connections{
      {"proxel1", "outport", "proxel2", "inport"}
  };

  auto graph = createGraph(factories, config, connections);

  Proxel::Ptr raw_ptr = graph.getProxel("proxel1");
  ASSERT_TRUE(raw_ptr != nullptr);

  auto ptr2 = graph.getProxel<TestProxel>("proxel2");
  ASSERT_TRUE(ptr2 != nullptr);
  ASSERT_EQ(21.0, ptr2->getStoredValue());

  graph.start();
  ASSERT_EQ(21.0, ptr2->getStoredValue());
  ASSERT_EQ(42.0, ptr2->getValue());
}

TEST(GraphFactory, createGraphWithout_getProxelFactory)
{
  using Plist = TestPropertyList;
  using MyProxel = TemplatedProxel<double>;

  const Factory<Plist> factory = MyProxel::create<Plist>;
  const FactoryMap<Plist> factories{{{"MyProxel", factory}}};

  const Plist properties1{{{"init_value", 42.0}}};
  const Plist properties2{{{"init_value", 21.0}}};

  const auto configs = std::vector<ProxelConfig<Plist>> {
      {"proxel1", "MyProxel", properties1},
      {"proxel2", "MyProxel", properties2}
  };

  const std::vector<ConnectionSpec> connections{
      {"proxel1", "outport", "proxel2", "inport"}
  };

  Graph graph = createGraph(factories, configs, connections);

  const Proxel::Ptr raw_ptr = graph.getProxel("proxel1");
  ASSERT_TRUE(raw_ptr != nullptr);

  const auto ptr2 = graph.getProxel<MyProxel>("proxel2");
  ASSERT_TRUE(ptr2 != nullptr);
  ASSERT_EQ(21.0, ptr2->getStoredValue());

  graph.start();
  ASSERT_EQ(21.0, ptr2->getStoredValue());
  ASSERT_EQ(42.0, ptr2->getValue());

}

TEST(GraphFactory, multiple_definitions_of_same_proxel_id_throws)
{
  using Plist = TestPropertyList;
  using MyProxel = TemplatedProxel<double>;

  const Factory<Plist> factory = MyProxel::create<Plist>;
  const FactoryMap<Plist> factories{{{"MyProxel", factory}}};

  const Plist properties1{{{"init_value", 42.0}}};
  const Plist properties2{{{"init_value", 21.0}}};

  const auto configs = std::vector<ProxelConfig<Plist>> {
      {"proxel1", "MyProxel", properties1},
      {"proxel1", "MyProxel", properties1},
      {"proxel2", "MyProxel", properties2}
  };

  const std::vector<ConnectionSpec> connections{
      {"proxel1", "outport", "proxel2", "inport"}
  };

  ASSERT_THROW(createGraph(factories, configs, connections), std::invalid_argument);
}

