// Copyright 2019, Forsvarets forskningsinstitutt. All rights reserved.
#include "superflow/proxel.h"
#include "superflow/loader/register_factory.h"
#include "fauxade.h"

namespace flow::test
{
class Mummy : public flow::Proxel
{
public:
  explicit Mummy(int i)
  {
    setState(State::Paused);
    setStatusInfo(std::to_string(i));
  }

  void start() override
  { setState(State::Running); }

  void stop() noexcept override
  { setState(State::Unavailable); }

  ~Mummy() override = default;

  template<typename PropertyList>
  static flow::Proxel::Ptr createWithArgs(const PropertyList&, const Fauxade::Ptr&);
};

template<typename PropertyList>
flow::Proxel::Ptr Mummy::createWithArgs(const PropertyList&, const Fauxade::Ptr& f_ptr)
{
  return std::make_shared<Mummy>(f_ptr->val);
}

REGISTER_PROXEL_FACTORY(Mummy, Mummy::createWithArgs)
}
