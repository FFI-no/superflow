// Copyright 2019, Forsvarets forskningsinstitutt. All rights reserved.
#include "superflow/utils/wait_for_signal.h"

#include "superflow/utils/signal_waiter.h"

namespace flow
{
void waitForSignal(const std::vector<int>& signals)
{
  const SignalWaiter waiter{signals};
  waiter.getFuture().wait();
}
}
