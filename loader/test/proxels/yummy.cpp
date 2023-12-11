// Copyright 2019, Forsvarets forskningsinstitutt. All rights reserved.
#include "superflow/proxel.h"
#include "superflow/loader/register_factory.h"
#include "superflow/value.h"

namespace flow::test
{
class Yummy : public flow::Proxel
{
public:
  Yummy(int value)
  {
    setState(State::AwaitingInput);
    setStatusInfo(std::to_string(value));
  }

  void start() override
  { setState(State::Running); }

  void stop() noexcept override
  { setState(State::Unavailable); }

  ~Yummy() override = default;
};

namespace
{
template<typename PropertyList>
flow::Proxel::Ptr freeFunctionCreate(const PropertyList& property_list)
{
  int value = flow::value<int>(property_list, "int key");
  return std::make_shared<Yummy>(value);
}

REGISTER_PROXEL_FACTORY(Yummy, freeFunctionCreate)
}
}
