// Copyright 2019, Forsvarets forskningsinstitutt. All rights reserved.
#include "superflow/utils/blocker.h"
#include "superflow/utils/throttle.h"

#include "gtest/gtest.h"

#include <string>
#include <thread>

using namespace flow;

TEST(Throttle, throttling)
{
  std::vector<std::string> result;
  Blocker blocker;

  const auto publish_fn = [&result, &blocker](std::string&& s)
  {
    result.push_back(s);
    blocker.release();
  };

  constexpr auto publish_rate = std::chrono::milliseconds(100);

  {
    Throttle<std::string> th(publish_fn, publish_rate);

    th.push("first"); // Push the first data.

    EXPECT_TRUE(blocker.block()); // Wait to ensure throttle has pushed.
    blocker.rearm(); // block can be called again
    // As soon at it has published,
    // do a double push so that skip1 should be overwritten before next publish
    th.push("skip1");
    th.push("second");

    EXPECT_TRUE(blocker.block()); // Wait to ensure throttle has pushed again
    blocker.rearm(); // block can be called again

    th.push("skip2");
    th.push("last");

    blocker.block();
    ASSERT_EQ(result.back(), "last");
  }

  ASSERT_EQ(result.size(), 3);
  ASSERT_EQ(result[0], "first");
  ASSERT_EQ(result[1], "second");
  ASSERT_EQ(result[2], "last");
}

inline const void* addr(const std::string& str)
{ return static_cast<const void*>(str.c_str()); }


TEST(Throttle, move_data)
{
  const std::string contents(30, 'x');
  std::string str = contents;
  const auto str_address = addr(str);

  ASSERT_NE(addr(contents), str_address);
  ASSERT_EQ(contents, str); // Can compare contents even if str is moved.

  std::string result;
  bool published = false;
  Blocker blocker;
  const auto publish_fn = [&result, &published, &blocker](auto&& s)
  {
    published = true;
    result = std::forward<decltype(s)>(s);
    blocker.release();
  };
  constexpr auto publish_rate = std::chrono::microseconds(1);

  Throttle<std::string> th(publish_fn, publish_rate);

  th.push(std::move(str));
  blocker.block();
  EXPECT_EQ(published, true);
  EXPECT_EQ(contents, result);
  EXPECT_EQ(str_address, addr(result));
}

TEST(Throttle, copy_data)
{
  const std::string str(30, 'x');
  const auto str_address = addr(str);

  std::string result;
  bool published = false;
  Blocker blocker;
  const auto publish_fn = [&result, &published, &blocker](auto&& s)
  {
    published = true;
    result = std::forward<decltype(s)>(s);
    blocker.release();
  };
  constexpr auto publish_rate = std::chrono::microseconds(1);

  Throttle<std::string> th(publish_fn, publish_rate);

  th.push(str);
  blocker.block();

  ASSERT_EQ(published, true);
  EXPECT_EQ(str, result);
  EXPECT_NE(str_address, addr(result));
}

TEST(Throttle, exception)
{
  Blocker blocker;
  const auto publish_fn = [&blocker](int)
  {
    unblockOnThreadExit(blocker);
    throw std::runtime_error("jalla jalla");
  };
  const auto publish_rate = std::chrono::microseconds(1);

  Throttle<int> th(publish_fn, publish_rate);
  EXPECT_NO_THROW(th.push(42));

  blocker.block();
  EXPECT_THROW(th.push(42), std::runtime_error);
}
