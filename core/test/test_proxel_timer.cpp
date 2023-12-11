// Copyright 2019, Forsvarets forskningsinstitutt. All rights reserved.
#include "superflow/utils/proxel_timer.h"

#include "gtest/gtest.h"

#include <chrono>
#include <thread>

using namespace flow;

TEST(ProxelTimer, stop_before_start_throws)
{
  ProxelTimer timer;
  ASSERT_THROW(timer.stop(), std::runtime_error);
}

TEST(ProxelTimer, start_before_stop_doesnt_throw)
{
  ProxelTimer timer;
  timer.start();
  ASSERT_NO_THROW(timer.stop());
}

TEST(ProxelTimer, peek)
{
  ProxelTimer timer;
  timer.start();
  double elapsed_time;
  ASSERT_NO_THROW(elapsed_time = timer.peek());
  ASSERT_GT(elapsed_time, 0.);
}

TEST(ProxelTimer, get_run_count)
{
  using namespace std::chrono_literals;
  {
    ProxelTimer timer;
    timer.start();
    timer.stop();
    timer.start();
    timer.stop();

    EXPECT_EQ(2, timer.getRunCount());
  }
}

TEST(ProxelTimer, get_busyness)
{
  using namespace std::chrono_literals;
  {
    ProxelTimer timer;
    timer.start();
    timer.stop();

    EXPECT_EQ(1, timer.getAverageBusyness());
  }

  {
    ProxelTimer timer;

    timer.start();
    std::this_thread::sleep_for(100us);
    timer.stop();

    std::this_thread::sleep_for(10us);

    timer.start();
    std::this_thread::sleep_for(100us);
    timer.stop();

    EXPECT_LE(0.1, timer.getAverageBusyness());
  }
}

TEST(ProxelTimer, get_average_processing_time)
{
  using namespace std::chrono_literals;

  {
    ProxelTimer timer;
    timer.start();
    std::this_thread::sleep_for(1ms);
    const auto total = timer.stop();
    const auto average = timer.getAverageProcessingTime();

    EXPECT_EQ(total, average);
  }
  {
    ProxelTimer timer;
    timer.start();
    std::this_thread::sleep_for(1ms);
    auto total = timer.stop();
    timer.start();
    total += timer.stop();

    const auto average = timer.getAverageProcessingTime();
    EXPECT_EQ(total/timer.getRunCount(), average);
  }
}

TEST(ProxelTimer, get_status_info)
{
  ProxelTimer timer;
  std::string info;
  EXPECT_NO_FATAL_FAILURE(info = timer.getStatusInfo());
  EXPECT_EQ(info.back(), '0');
}
