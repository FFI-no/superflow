// Copyright 2023, Forsvarets forskningsinstitutt. All rights reserved.
#include "superflow/loader/register_factory.h"
#include "superflow/proxel.h"
#include "superflow/value.h"

namespace flow::test
{
class YamlTestProxel : public flow::Proxel
{
public:
  YamlTestProxel(int value)
  {
    setState(State::AwaitingInput);
    setStatusInfo(std::to_string(value));
  }

  void start() override
  { setState(State::Running); }

  void stop() noexcept override
  { setState(State::Unavailable); }

  ~YamlTestProxel() override = default;
};

namespace
{
template<typename PropertyList>
flow::Proxel::Ptr createYamlTestProxel(const PropertyList& property_list)
{
  int value = flow::value<int>(property_list, "int");
  return std::make_shared<YamlTestProxel>(value);
}

REGISTER_PROXEL_FACTORY(YamlTestProxel, createYamlTestProxel)
}
}
