// Copyright 2020, Forsvarets forskningsinstitutt. All rights reserved.
#pragma once

#include <condition_variable>
#include <functional>
#include <future>

namespace flow
{
/// \brief Calls a provided function on a separate thread at a given
/// interval, omitting the first (immediate) call. Will continue to
/// do so forever, until `stop()` or dtor is called. Useful for instance for
/// printing error messages for stalling tasks.
///
/// ```cpp
/// Metronome repeater{
///   [](const auto& duration)
///   {
///     std::cerr
///       << "my_func has been stalling for " << duration.count() << "s"
///       << std::endl;
///   },
///   std::chrono::seconds{2}
/// };
///
/// my_func(); // we expect that this might stall
/// repeater.stop();
///
/// // `repeater` will print the above message every 2 seconds until
/// // `my_func()` has returned and `stop()` has been called.
/// ```
///
/// If `my_func()` throws an exception, the repeater will stop,
/// and a subsequent call to `check()` will rethrow the exception.
///
/// \see Sleeper, Throttle
class Metronome
{
public:
  using Duration = std::chrono::steady_clock::duration;
  using Functional = std::function<void(const Duration&)>;

  Metronome(
    const Functional& func,
    Duration period
  );

  ~Metronome();

  /// Wait for the worker to finish, and also receive any exceptions
  /// thrown in the worker thread (inside `func`).
  void get();

  /// Method for testing if `func` has thrown an exception.
  /// Re-throws the exception thrown by `func`, if it has thrown.
  /// Otherwise returns immediately.
  void check();

  /// Stop calling the provided `func` (and unblock dtor)
  void stop() noexcept;

private:
  bool has_stopped_;
  std::mutex mutex_;
  std::condition_variable cv_;
  std::future<void> worker_;
};
}