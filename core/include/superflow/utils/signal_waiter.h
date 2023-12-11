// Copyright 2019, Forsvarets forskningsinstitutt. All rights reserved.
#pragma once

#include <condition_variable>
#include <csignal>
#include <future>
#include <mutex>
#include <vector>

namespace flow
{
/// \brief RAII-wrapper for thread-safe listening to one or more signals.
/// If you are using std::signal() or plain C signal() in your program, this class might not work as expected.
/// Use SignalWaiter::hasGottenSignal() at any time to check if one of the signals have been raised.
/// Use e.g. SignalWaiter::getFuture().wait() to wait until a signal has been received.
class SignalWaiter
{
public:
  /// \brief Creates a SignalWaiter that listens to all `signals`.
  /// The signal handlers are guaranteed to have been installed once the CTOR returns.
  explicit SignalWaiter(
    const std::vector<int>& signals = {SIGINT}
  );

  /// \brief Immediately stops listening to signals, as well as resolving all waiting futures
  ~SignalWaiter();

  /// \brief Returns whether any of the `signals` have been received
  [[nodiscard]] bool hasGottenSignal() const;

  /// \brief Returns a future that is resolved as soon as one of the `signals` have been received, or the waiter
  /// has been destroyed.
  [[nodiscard]] std::shared_future<void> getFuture() const;

private:
  std::mutex mutex_;
  std::condition_variable cv_;

  bool is_waiting_;
  bool got_signal_;
  std::shared_ptr<std::function<void()>> handler_;
  std::shared_future<void> worker_;

  void handleSignal();

  void await(const std::vector<int>& signals);
};
}

