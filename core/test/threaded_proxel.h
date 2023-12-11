// Copyright 2019, Forsvarets forskningsinstitutt. All rights reserved.
#pragma once

#include "superflow/proxel.h"

#include <thread>

namespace flow
{
class ThreadedProxel : public Proxel
{
public:
  ThreadedProxel()
      : thread_id_{}
      , start_was_called_{false}
      , stop_was_called_{false}
  {}

  using Ptr = std::shared_ptr<ThreadedProxel>;

  void start() override
  {
    start_was_called_ = true;
    thread_id_ = std::this_thread::get_id();
  }

  void stop() noexcept override
  {
    stop_was_called_ = true;
  }

  std::thread::id getThreadId() const
  {
    return thread_id_;
  }

  bool startWasCalled() const
  {
    return start_was_called_;
  }

  bool stopWasCalled() const
  {
    return stop_was_called_;
  }

private:
  std::thread::id thread_id_;
  bool start_was_called_;
  bool stop_was_called_;
};
}
