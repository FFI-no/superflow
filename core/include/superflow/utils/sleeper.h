// Copyright 2022, Forsvarets forskningsinstitutt. All rights reserved.
#pragma once

#include <chrono>

namespace flow
{
/// \brief Being the sister of Metronome and Throttle, Sleeper is the third way of rate control.
/// Typically intended to be used within a for-loop in order to stall execution.
/// Sleeper will sleep until another `period` has passed since the previous call to `sleepForRemainderOfPeriod`, thus a steady rate.
/// You can also adjust the sleep period with ´setNewSleepPeriod(Clock::duration period);`
///
/// ```cpp
/// using namespace std::chrono_literals;
/// const Sleeper rate_limiter(10ms);
///
/// for (const auto& data : *latched_port_)
/// {
///   do_work();
///   rate_limiter.sleepForRemainderOfPeriod();  // Sleep for the rest of the period
///
///   if(some condition)
///   {
///      rate_limiter.setNewSleepPeriod(xms);
///   }
/// }
/// ```
///
///
/// If execution time of `do_work()` is less than 10ms, it will not be called again until 10ms has passed since
/// the previous call. If execution time is > 10ms, it will be called again immediately as sleepForRemainderOfPeriod time is already due.
///
/// \see Metronome, Throttle
class Sleeper
{
public:
  using Clock = std::chrono::steady_clock;

  explicit Sleeper(Clock::duration period);

  void sleepForRemainderOfPeriod() const;
  void setSleepPeriod(const Clock::duration period);

private:
  mutable Clock::time_point time_point_;
  Clock::duration period_;
};
}
