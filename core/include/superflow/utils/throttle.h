// Copyright 2019, Forsvarets forskningsinstitutt. All rights reserved.
#pragma once

#include <chrono>
#include <condition_variable>
#include <functional>
#include <future>
#include <mutex>

namespace flow
{
/// \brief This is a utility for limiting the rate of which a function is called.
///
/// The target function will be called periodically at a requested rate, using an
/// internal timer.
/// If new data is provided repeatedly while the throttler is stalling, only the latest data
/// will be buffered. When the timer expires, that last data will be sent.
/// If the timer expires without any new data being available, nothing happens.
/// If new data is provided after the timer expires, it is sent immediately
/// and the timer starts again.
/// \tparam T the type of data sent.
///
/// \see Metronome, Sleeper
template<typename T>
class Throttle
{

public:
  /// Signature of the target function
  using Callback = std::function<void(T&&)>;

  using Duration = std::chrono::steady_clock::duration;

  /// \brief Create a new Throttle.
  /// If the callback throws an exception, it will be caught and propagated
  /// to the next caller of push.
  /// \param cb the function to be called if data is available when the timer expires.
  /// \param delay the rate at which the given function is called
  Throttle(Callback cb, const Duration& delay);

  ~Throttle();

  /// \brief Push new data to the Throttle.
  /// \param data to be sent to the target function.
  /// \throws any exception that may occur in the provided callback function.
  /// If an exception has been raised in the previous run of the worker thread, this function
  /// will propagate the exception to the current caller.
  template<class U>
  void push(U&& data);

private:
  bool stopped_ = false;
  bool new_data_ = false;
  std::mutex mu_;
  std::condition_variable cv_;
  T data_;
  Callback callback_;
  Duration delay_;
  std::future<void> runner_;

  void run();
};

// --- Implementation --- //

template<typename T>
Throttle<T>::Throttle(Callback cb, const Duration& delay)
    : callback_{std::move(cb)}
    , delay_{delay}
    , runner_{std::async(std::launch::async, [this]{ run(); })}
{}

template<typename T>
Throttle<T>::~Throttle()
{
  stopped_ = true;
  cv_.notify_all();
}

template<typename T>
void Throttle<T>::run()
{
  while (!stopped_)
  {
    std::unique_lock<std::mutex> lock(mu_);
    cv_.wait(lock, [this]{ return (stopped_ || new_data_); });

    if (stopped_)
    { break; }

    try
    { callback_(std::move(data_)); }
    catch (...)
    {
      stopped_ = true;
      throw;
    }
    new_data_ = false;
    cv_.wait_for(lock, delay_, [this]{return stopped_;});
  }
}

template<typename T>
template<typename U>
void Throttle<T>::push(U&& data)
{
  if (stopped_)
  { runner_.get(); }
  {
    std::scoped_lock lock(mu_);
    data_ = std::forward<U>(data);
    new_data_ = true;
  }
  cv_.notify_one();
}
}
