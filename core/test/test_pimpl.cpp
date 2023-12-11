// Copyright (c) 2020 Forsvarets forskningsinstitutt (FFI). All rights reserved.
#include "pimpl_test.h"

#include "gtest/gtest.h"

TEST(SuperFlowCurses, pimpl_empty_ctor)
{
  // This works if explicit instantiation is done for the impl-class.
  // See pimpl_test.cpp#6.
  // There should be no linker errors and no
  // "undefined reference to 'pimpl<flow::test::A::impl>::~pimpl()'"
  flow::test::A a;
}