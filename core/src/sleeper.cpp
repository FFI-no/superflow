// Copyright (c) 2022, Forsvarets forskningsinstitutt. All rights reserved.
#include "superflow/utils/sleeper.h"
#include <thread>

namespace flow
{

Sleeper::Sleeper(Clock::duration period)
  : time_point_{Clock::now()}
  , period_(period)
{}

void Sleeper::sleepForRemainderOfPeriod() const {
  time_point_ += period_;
  std::this_thread::sleep_until(time_point_);
}

void Sleeper::setSleepPeriod(const Clock::duration period) {
  period_ = period;
}
}
