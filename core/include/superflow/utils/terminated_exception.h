// Copyright 2019, Forsvarets forskningsinstitutt. All rights reserved.
#pragma once

#include <stdexcept>

namespace flow
{
struct TerminatedException : public std::runtime_error
{
  TerminatedException()
      : std::runtime_error("LockQueue is terminated")
  {}

  explicit TerminatedException(const std::string& what_arg)
      : std::runtime_error(what_arg)
  {}

  explicit TerminatedException(const char* what_arg)
      : std::runtime_error(what_arg)
  {}
};
}
