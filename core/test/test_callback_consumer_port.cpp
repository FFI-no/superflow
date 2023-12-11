// Copyright 2019, Forsvarets forskningsinstitutt. All rights reserved.
#include "superflow/callback_consumer_port.h"
#include "superflow/policy.h"
#include "superflow/producer_port.h"

#include "gtest/gtest.h"

#include <future>
#include <thread>

using namespace flow;

TEST(CallbackConsumerPort, DataIsReceived)
{
  using Producer = ProducerPort<int>;

  std::promise<int> promise;

  auto producer = std::make_shared<Producer>();
  auto consumer = std::make_shared<CallbackConsumerPort<int>>(
      [&promise](int i)
      { promise.set_value(i); });

  ASSERT_NO_THROW(producer->connect(consumer));

  constexpr int value = 42;
  producer->send(value);

  ASSERT_EQ(value, promise.get_future().get());
}

namespace
{
class Base
{
public:
  virtual ~Base() = default;
};

class Derived : public Base
{};
}

TEST(CallbackConsumer, upCast)
{
  using Consumer = CallbackConsumerPort<Base, ConnectPolicy::Single, Derived>;
  bool got_base;

  const auto consumer = std::make_shared<Consumer>(
    [&got_base](const Base& b)
    {
      got_base = true;
    }
  );

  got_base = false;
  consumer->receive(Base{}, nullptr);
  ASSERT_TRUE(got_base);

  const auto producer = std::make_shared<ProducerPort<Derived>>();
  consumer->connect(producer);

  got_base = false;
  producer->send(Derived{});
  ASSERT_TRUE(got_base);
}

TEST(CallbackConsumer, variantReceive)
{
  using Consumer = CallbackConsumerPort<std::variant<int, std::string>>;

  int received_int;
  std::string received_string;

  Consumer consumer{
    [&received_int, &received_string](const auto& variant)
    {
      if (std::holds_alternative<int>(variant))
      {
        received_int = std::get<int>(variant);
      }
      else if (std::holds_alternative<std::string>(variant))
      {
        received_string = std::get<std::string>(variant);
      }
    }
  };

  {
    received_int = -1;
    received_string = "";
    consumer.receive(10, nullptr);
    ASSERT_EQ(received_int, 10);
    ASSERT_EQ(received_string, "");
  }

  {
    received_int = -1;
    received_string = "";
    consumer.receive("hei", nullptr);
    ASSERT_EQ(received_int, -1);
    ASSERT_EQ(received_string, "hei");
  }
}

TEST(CallbackConsumer, variantConnect)
{
  using Consumer = CallbackConsumerPort<std::variant<int, std::string>>;

  int received_int;
  std::string received_string;

  const auto consumer = std::make_shared<Consumer>(
    [&received_int, &received_string](const auto& variant)
    {
      if (std::holds_alternative<int>(variant))
      {
        received_int = std::get<int>(variant);
      }
      else if (std::holds_alternative<std::string>(variant))
      {
        received_string = std::get<std::string>(variant);
      }
    }
  );

  {
    const auto producer = std::make_shared<ProducerPort<int>>();

    ASSERT_NO_THROW(consumer->connect(producer));
    producer->disconnect();
  }

  {
    const auto producer = std::make_shared<ProducerPort<std::string>>();

    ASSERT_NO_THROW(consumer->connect(producer));
    producer->disconnect();
  }
}

TEST(CallbackConsumer, numTransactions)
{
  using Producer = ProducerPort<int>;
  using Consumer = CallbackConsumerPort<int>;

  std::promise<int> promise;
  std::mutex mu;
  std::condition_variable cv;
  std::atomic_bool tests_verified = false;

  auto producer = std::make_shared<Producer>();
  auto consumer = std::make_shared<Consumer>(
    [&promise, &mu, &cv, &tests_verified](int i)
    {
      std::unique_lock<std::mutex> mlock(mu);
      cv.wait(mlock, [&tests_verified](){ return tests_verified.load(); });
      promise.set_value(i);
    }
  );

  ASSERT_NO_THROW(producer->connect(consumer));

  ASSERT_EQ(0, producer->getStatus().num_transactions);
  ASSERT_EQ(0, consumer->getStatus().num_transactions);

  constexpr int value = 42;
  auto pusher = std::async(
    std::launch::async,
    [&producer,&value]()
    {
      ASSERT_NO_THROW(producer->send(value));
    }
  );

  std::this_thread::sleep_for(std::chrono::milliseconds(5));
  EXPECT_EQ(1, producer->getStatus().num_transactions);
  EXPECT_EQ(0, consumer->getStatus().num_transactions);

  {
    std::unique_lock<std::mutex> mlock(mu);
    tests_verified = true;
    cv.notify_one();
  }

  pusher.wait();
  ASSERT_EQ(value, promise.get_future().get());
  EXPECT_EQ(1, consumer->getStatus().num_transactions);
}