// Copyright 2019, Forsvarets forskningsinstitutt. All rights reserved.
#include "superflow/utils/proxel_timer.h"

#include <iomanip>
#include <sstream>

namespace flow
{
void ProxelTimer::start()
{
  start_ = Clock::now();
  std::call_once(first_flag_, [this]()
  { first_time_point_ = start_; });
}

double ProxelTimer::stop()
{
  if (Duration(first_time_point_.time_since_epoch()).count() == 0.)
  { throw std::runtime_error("ProxelTimer::stop() has been called before ProxelTimer::start()"); }

  const auto now = Clock::now();

  const double processing_time = Duration{now - start_}.count();
  summed_processing_time_ += processing_time;

  ++run_counter_;
  mean_processing_time_ = summed_processing_time_ / static_cast<double>(run_counter_);

  const auto uptime = Duration{now - first_time_point_}.count();
  mean_busyness_time_ = summed_processing_time_ / uptime;

  return processing_time;
}

double ProxelTimer::peek() const
{
  const auto now = Clock::now();
  const double processing_time = Duration{now - start_}.count();
  return processing_time;
}

double ProxelTimer::getAverageProcessingTime() const
{
  return mean_processing_time_;
}

double ProxelTimer::getAverageBusyness() const
{
  return mean_busyness_time_;
}

unsigned long long ProxelTimer::getRunCount() const
{
  return run_counter_;
}

std::string ProxelTimer::getStatusInfo() const
{
  std::ostringstream ss;
  ss << std::fixed << std::setprecision(3)
     << "time: " << getAverageProcessingTime() << "s"
     << "\n"
     << "busy: " << getAverageBusyness();

  return ss.str();
}
}
