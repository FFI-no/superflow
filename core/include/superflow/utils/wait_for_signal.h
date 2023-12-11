// Copyright 2019, Forsvarets forskningsinstitutt. All rights reserved.
#pragma once

#include <csignal>
#include <vector>

namespace flow
{
/// \brief Make the current thread wait for the specified signal(s).
/// This is thread-safe and can be used on multiple threads simultaneously.
/// \param signals The signals to wait for (default is SIGINT).
void waitForSignal(const std::vector<int>& signals = {SIGINT});
}
