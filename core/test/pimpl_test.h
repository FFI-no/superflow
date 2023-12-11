// Copyright (c) 2020 Forsvarets forskningsinstitutt (FFI). All rights reserved.
#pragma once

#include "superflow/utils/pimpl_h.h"

namespace flow::test
{
class A
{
public:
  A();

private:
  class impl;

  flow::pimpl<impl> m_;
};
}