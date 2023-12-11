// Copyright 2019, Forsvarets forskningsinstitutt. All rights reserved.
#include "superflow/proxel.h"
#include "testing/dummy_value_adapter.h"

namespace flow::test
{
class Dummy : public flow::Proxel
{
public:
  Dummy()
  { setState(State::Paused); }

  void start() override
  { setState(State::Running); }

  void stop() noexcept override
  { setState(State::Unavailable); }

  ~Dummy() override = default;

  template<typename PropertyList>
  static flow::Proxel::Ptr static_create(const PropertyList&);
};

template<typename PropertyList>
flow::Proxel::Ptr Dummy::static_create(const PropertyList&)
{
  return std::make_shared<Dummy>();
}

REGISTER_DUMMY_PROXEL_FACTORY(Dummy, Dummy::static_create)
}
