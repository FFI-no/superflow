// Copyright (c) 2022, Forsvarets forskningsinstitutt. All rights reserved.
#include "superflow/utils/sleeper.h"

#include "gtest/gtest.h"

#include <thread>

namespace flow
{
TEST(Sleeper, check)
{
  using namespace std::chrono_literals;


  Sleeper sleeper(10ms);
  const auto begin = Sleeper::Clock::now();
  for (int i =0; i<10; ++i)
  {
    if (i > 4)
    {
      sleeper.setSleepPeriod(5ms);
    }
    sleeper.sleepForRemainderOfPeriod();

  }
  const auto end = Sleeper::Clock::now();
  const auto expected_duration =  5*(10ms + 5ms);
  const auto actual_duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - begin);
  //std::cout << "expected_duration: " << expected_duration.count() << std::endl;
  //std::cout << "actual_duration: " << actual_duration.count() << std::endl;
  ASSERT_TRUE(actual_duration == expected_duration);}
}
