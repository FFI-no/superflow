// Copyright (c) 2020 Forsvarets forskningsinstitutt (FFI). All rights reserved.
#include "superflow/utils/metronome.h"

#include "gtest/gtest.h"

#include <thread>

using namespace std::chrono_literals;

TEST(Metronome, check)
{
  {
    std::promise<void> promise;
    std::atomic_bool promise_isset{false};

    flow::Metronome non_crashing_repeater{
      [&promise,&promise_isset](const auto)
      {
        if (promise_isset)
        { return ; }

        promise.set_value();
        promise_isset = true;
      },
      1us
    };

    const auto future_status =  promise.get_future().wait_for(1s);
    ASSERT_NE(future_status, std::future_status::timeout);

    EXPECT_NO_THROW(non_crashing_repeater.check());
  }

  {
    std::promise<void> promise;
    flow::Metronome crashing_repeater{
      [&promise](const auto)
      {
        promise.set_value();
        throw std::runtime_error{"error"};
      },
      std::chrono::microseconds{1}
    };

    const auto future_status =  promise.get_future().wait_for(1s);
    ASSERT_NE(future_status, std::future_status::timeout);
    std::this_thread::sleep_for(10ms);

    EXPECT_THROW(crashing_repeater.check(), std::runtime_error);
  }
}

TEST(Metronome, get)
{
  {
    std::promise<void> promise;

    flow::Metronome metronome{
      [&promise](const auto)
      {
        static bool once = std::invoke([&promise]{ promise.set_value(); return true; });
      },
      std::chrono::microseconds{1}
    };

    const auto future_status =  promise.get_future().wait_for(1s);
    ASSERT_NE(future_status, std::future_status::timeout);

    metronome.stop();
    EXPECT_NO_THROW(metronome.get());
    EXPECT_THROW(metronome.get(), std::runtime_error);
  }
}

TEST(Metronome, stop)
{
  {
    flow::Metronome metronome{
      [](auto){},
      1us
    };

    metronome.stop();
    EXPECT_NO_THROW(metronome.get());
  }
}
