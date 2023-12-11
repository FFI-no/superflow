// Copyright (c) 2022 Forsvarets forskningsinstitutt (FFI). All rights reserved.

#pragma once

#include <atomic>
#include <condition_variable>
#include <functional>
#include <mutex>
#include <iostream>

namespace flow
{
/// \brief Class for controlling flow when doing multi threaded testing. Blocker::block() blocks the current thread until
/// Blocker::release() is called in another thread. This can be used to make the main thread wait for some computation in
/// another thread.
struct Blocker
{
  /// \brief Block the current thread until another thread calls release()
  /// \return return false if the thread was already released so no blocking was necessary,
  /// or true if an actual block was experienced.
  bool block()
  {
    if (is_released_)
    { return false; } // did not wait

    std::unique_lock lock{mutex_};
    cv_.wait(lock, [this]
      { return static_cast<bool>(is_released_); }
    );

    return true;
  }

  /// \brief Unblock the thread(s) that has called block()
  void release()
  {
    is_released_ = true;
    cv_.notify_all();
  }

  /// \brief Resets the Blocker. Further calls to block() will block until someone again calls release()
  void rearm()
  { is_released_ = false; }

  bool isReleased() const
  { return is_released_; }

private:
  mutable std::mutex mutex_{};
  mutable std::condition_variable cv_;
  std::atomic_bool is_released_{false};
};

/// \brief Function that calls Blocker::release() whenever the thread exits. Useful when testing uncontrolled exits from
/// the thread, e.g exceptions.
inline void unblockOnThreadExit(Blocker& blocker)
{
  class Unblocker
  {
  public:
    explicit Unblocker(Blocker& blocker) : blocker_{blocker}
    {}

    ~Unblocker()
    {
      blocker_.release();
    }

  private:
    Blocker& blocker_;
  };

  thread_local Unblocker unblocker(blocker);
}
}
