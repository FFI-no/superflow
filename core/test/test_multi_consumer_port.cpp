// Copyright 2019, Forsvarets forskningsinstitutt. All rights reserved.
#include "superflow/multi_consumer_port.h"
#include "superflow/producer_port.h"

#include "gtest/gtest.h"

#include <chrono>
#include <future>
#include <thread>

using namespace flow;

TEST(MultiConsumer, receive)
{
  using Producer = ProducerPort<int>;

  constexpr size_t num_producers = 10;
  auto consumer = std::make_shared<MultiConsumerPort<int, flow::GetMode::Latched>>();

  std::vector<std::shared_ptr<Producer>> producers;

  for (size_t i = 0; i < num_producers; ++i)
  {
    producers.push_back(std::make_shared<Producer>());
  }

  for (auto& producer : producers)
  {
    ASSERT_NO_THROW(producer->connect(consumer));
    ASSERT_NO_THROW(producer->send(42));
  }

  for (size_t i = 0; i < 10; ++i)
  {
    auto items = consumer->get();

    for (const auto item : items)
    {
      ASSERT_EQ(item, 42);
    }
  }
}

TEST(MultiConsumer, emptyConsumerReceivesNothing)
{
  auto consumer = std::make_shared<MultiConsumerPort<int, flow::GetMode::Latched>>();
  auto items = consumer->get();

  ASSERT_TRUE(items.empty());
}

TEST(MultiConsumer, deactivateCausesThrow)
{
  auto consumer = std::make_shared<MultiConsumerPort<int, flow::GetMode::Latched>>();
  auto producer = std::make_shared<ProducerPort<int>>();

  consumer->connect(producer);
  consumer->deactivate();

  ASSERT_THROW(consumer->get(), std::runtime_error);
}

TEST(MultiConsumer, receiveIsLatched)
{
  using Producer = ProducerPort<int>;

  constexpr size_t num_producers = 10;
  auto consumer = std::make_shared<MultiConsumerPort<int, flow::GetMode::Latched>>();

  std::vector<std::shared_ptr<Producer>> producers;

  for (size_t i = 0; i < num_producers; ++i)
  {
    producers.push_back(std::make_shared<Producer>());
  }

  for (auto& producer : producers)
  {
    ASSERT_NO_THROW(producer->connect(consumer));
  }

  // send data for all but one producer
  for (size_t i = 0; i < num_producers - 1; ++i)
  {
    producers[i]->send(42);
  }

  bool got_data = false;
  auto worker = std::async(std::launch::async, [&got_data, &consumer]()
  {
    auto item = consumer->get(); // expected to throw when deactivate() is called
    got_data = true; // which means this won't happen
  });

  std::this_thread::sleep_for(std::chrono::milliseconds{10});
  ASSERT_FALSE(got_data);

  consumer->deactivate();

  ASSERT_THROW(worker.get(), std::runtime_error);
  ASSERT_FALSE(got_data);
}

TEST(MultiConsumer, receiveReadyOnly)
{
  using Producer = ProducerPort<int>;

  constexpr size_t num_producers = 10;
  auto consumer = std::make_shared<MultiConsumerPort<int, flow::GetMode::ReadyOnly>>();

  std::vector<std::shared_ptr<Producer>> producers;

  for (size_t i = 0; i < num_producers; ++i)
  {
    producers.push_back(std::make_shared<Producer>());
  }

  for (auto& producer : producers)
  {
    ASSERT_NO_THROW(producer->connect(consumer));
  }

  // send data for all but one producer
  for (size_t i = 0; i < num_producers - 1; ++i)
  {
    producers[i]->send(static_cast<int>(13 * (i + 1)));
  }

  const auto ready_items = consumer->get();

  ASSERT_EQ(ready_items.size(), num_producers - 1);

  for (const int item : ready_items)
  {
    ASSERT_NE(item, static_cast<int>(13 * num_producers));
  }
}

TEST(MultiConsumer, deactivateCausesOperatorFalse)
{
  MultiConsumerPort<int> consumer;

  ASSERT_TRUE(consumer);

  consumer.deactivate();

  ASSERT_FALSE(consumer);
}

TEST(MultiConsumer, reportsNumConnections)
{
  const auto consumer = std::make_shared<MultiConsumerPort<int>>();
  constexpr size_t num_producers = 10;

  for (size_t i = 0; i < num_producers; ++i)
  {
    consumer->connect(std::make_shared<ProducerPort<int>>());
  }

  ASSERT_EQ(consumer->getStatus().num_connections, num_producers);
}

TEST(MultiConsumer, clear)
{
  const auto consumer = std::make_shared<MultiConsumerPort<int>>(2);

  const auto producer1 = std::make_shared<ProducerPort<int>>();
  const auto producer2 = std::make_shared<ProducerPort<int>>();

  consumer->connect(producer1);
  consumer->connect(producer2);

  ASSERT_FALSE(consumer->hasNext());
  producer1->send(0);
  ASSERT_FALSE(consumer->hasNext());
  producer2->send(0);
  ASSERT_TRUE(consumer->hasNext());
  consumer->clear();
  ASSERT_FALSE(consumer->hasNext());
  producer1->send(1);
  ASSERT_FALSE(consumer->hasNext());
  producer2->send(1);
  ASSERT_TRUE(consumer->hasNext());

  const auto values = consumer->get();

  ASSERT_EQ(values.size(), 2u);
  ASSERT_EQ(values[0], 1);
  ASSERT_EQ(values[1], 1);
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

TEST(MultiConsumer, upCast)
{
  using Consumer = MultiConsumerPort<Base, GetMode::Blocking, Derived>;
  const auto consumer = std::make_shared<Consumer>(1);

  {
    const auto producer = std::make_shared<ProducerPort<Base>>();

    ASSERT_NO_THROW(consumer->connect(producer));

    producer->send(Base{});
    ASSERT_TRUE(consumer->getNext().has_value());
    ASSERT_FALSE(consumer->hasNext());
    producer->disconnect();
  }

  {
    const auto producer = std::make_shared<ProducerPort<Derived>>();

    ASSERT_NO_THROW(consumer->connect(producer));

    producer->send(Derived{});
    ASSERT_TRUE(consumer->getNext().has_value());
    ASSERT_FALSE(consumer->hasNext());
    producer->disconnect();
  }
}

TEST(MultiConsumer, variantReceive)
{
  using Consumer = MultiConsumerPort<std::variant<int, std::string>>;
  Consumer consumer{1};

  consumer.receive(10, nullptr);

  {
    const auto opt_value = consumer.getNext();
    ASSERT_TRUE(opt_value.has_value());

    const auto& variant = opt_value.value().front();
    ASSERT_TRUE(std::holds_alternative<int>(variant));
    ASSERT_EQ(std::get<int>(variant), 10);
  }

  ASSERT_FALSE(consumer.hasNext());

  consumer.receive("hei", nullptr);

  {
    const auto opt_value = consumer.getNext();
    ASSERT_TRUE(opt_value.has_value());

    const auto& variant = opt_value.value().front();
    ASSERT_TRUE(std::holds_alternative<std::string>(variant));
    ASSERT_EQ(std::get<std::string>(variant), "hei");
  }
}

TEST(MultiConsumer, variantConnect)
{
  using Consumer = MultiConsumerPort<std::variant<int, std::string>>;
  const auto consumer = std::make_shared<Consumer>(1);

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

TEST(MultiConsumer, numTransactions)
{
  using Producer = ProducerPort<int>;
  using Consumer = MultiConsumerPort<int, flow::GetMode::Latched>;

  constexpr size_t num_producers = 10;
  auto consumer = std::make_shared<Consumer>();

  std::vector<Producer::Ptr> producers;

  for (size_t i = 0; i < num_producers; ++i)
  {
    producers.push_back(std::make_shared<Producer>());
  }

  for (auto& producer : producers)
  {
    ASSERT_NO_THROW(producer->connect(consumer));
    ASSERT_NO_THROW(producer->send(42));
    ASSERT_EQ(1, producer->getStatus().num_transactions);
  }

  ASSERT_EQ(0, consumer->getStatus().num_transactions);
  for (size_t i = 0; i < 10; ++i)
  {
    auto items = consumer->get();
    ASSERT_EQ(i+1, consumer->getStatus().num_transactions);
    ASSERT_EQ(1, producers[i]->getStatus().num_transactions);
  }
}