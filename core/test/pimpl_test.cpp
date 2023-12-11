// Copyright (c) 2020 Forsvarets forskningsinstitutt (FFI). All rights reserved.

#include "pimpl_test.h"
#include "superflow/utils/pimpl_impl.h"

template class flow::pimpl<flow::test::A::impl>;

namespace flow::test
{
class A::impl
{};

A::A()
    : m_{}
{}
}