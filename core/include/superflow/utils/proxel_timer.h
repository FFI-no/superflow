// Copyright 2019, Forsvarets forskningsinstitutt. All rights reserved.
#pragma once

#include "superflow/proxel_status.h"

#include <atomic>
#include <chrono>
#include <mutex>
#include <string>

namespace flow
{
/// \brief Utility class for measuring the workload of a Proxel.
///
/// Typical usage:
/// \code{.cpp}
/// flow::ProxelTimer proxel_timer_;
/// // ...
/// for (const auto& data: *input_port_)
/// {
///   setState(State::Running);
///   proxel_timer_.start();
///   // work ...
///   proxel_timer_.stop();
///   setStatusInfo(proxel_timer_.getStatusInfo());
/// }
/// \endcode
class ProxelTimer
{
public:
  /// \brief Start the timer
  void start();

  /// \brief Stop the timer
  /// \return elapsed time since start (in seconds)
  double stop();

  /// \brief Read time since start without stopping the timer
  /// \return elapsed time since start (in seconds)
  [[nodiscard]] double peek() const;

  /// \brief Get time from start to stop, averaged over all starts and stops.
  /// Measured in seconds.
  /// \return average processing time (in seconds)
  [[nodiscard]] double getAverageProcessingTime() const;

  /// \brief Get the ratio of processing vs idle state.
  ///
  /// A value of 1 is max busyness and means no idle time.
  /// A value of 0 means no processing at all, only idle time.
  ///
  /// Computed as summed processing time divided by time since start was called for the first time.<br/>
  /// \f[ Busyness = \frac{\sum_{i} {stop_i-start_i}}{stop_i-start_0} \f]
  /// \return average busy ratio
  [[nodiscard]] double getAverageBusyness() const;

  /// \brief Read how many times the timer has been stopped.
  /// \return run count
  [[nodiscard]] unsigned long long getRunCount() const;

  /// \brief Create a formatted string containing average processing time and average busyness.
  /// \return The formatted string.
  [[nodiscard]] std::string getStatusInfo() const;

private:
  using Clock = std::chrono::high_resolution_clock;
  using Duration = std::chrono::duration<double>;

  std::once_flag first_flag_;
  Clock::time_point first_time_point_;

  std::atomic<unsigned long long> run_counter_{0};
  double summed_processing_time_{0};

  std::atomic<double> mean_processing_time_{0};
  std::atomic<double> mean_busyness_time_{0};

  Clock::time_point start_;
};
}
