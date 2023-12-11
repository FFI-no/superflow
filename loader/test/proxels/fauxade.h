// Copyright 2019, Forsvarets forskningsinstitutt. All rights reserved.
#pragma once

#include <memory>

namespace flow::test
{
struct Fauxade
{
  using Ptr = std::shared_ptr<Fauxade>;
  int val = 0;
  explicit Fauxade(const int i) : val{i}{}
};
}
