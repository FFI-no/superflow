#pragma once

#include "superflow/loader/register_factory.h"
#include "boost/preprocessor/stringize.hpp"

// By using REGISTER_PROXEL_FACTORY_SECTIONED,
// you can register several adapter types at once, and then choose one of the adapters when
// loading the library.
//
// Preferably, a helper macro is wrapping REGISTER_PROXEL_FACTORY_SECTIONED as in this example.
// If the Adapter authors provide similar macros, you could go on like so:
//
// #include "my/json_property_list.h"
// REGISTER_JSON_PROXEL_FACTORY(Dummy, Dummy::static_create)
//
// #include "superflow/yaml/yaml_property_list.h"
// REGISTER_YAML_PROXEL_FACTORY(Dummy, Dummy::static_create)
//
// #include "testing/dummy_value_adapter.h"
// REGISTER_DUMMY_PROXEL_FACTORY(Dummy, Dummy::static_create)

#define REGISTER_DUMMY_PROXEL_FACTORY(ProxelName, Factory) \
        REGISTER_PROXEL_FACTORY_SECTIONED(ProxelName, Factory<flow::test::DummyValueAdapter>, DUMMY_ADAPTER_NAME)

namespace flow::test
{
class DummyValueAdapter
{
public:
  template<typename R>
  R convertValue(const std::string&) const
  { return R{}; }

  [[nodiscard]] bool hasKey(const std::string& key) const;

  static constexpr const char* adapter_name{BOOST_PP_STRINGIZE(DUMMY_ADAPTER_NAME)};

  std::string key_{"dummy_key"};
  int int_value{21};
};

template<>
inline int DummyValueAdapter::convertValue<int>(const std::string& key) const
{
  return key == "int key" ? int_value : -1;
}
}
