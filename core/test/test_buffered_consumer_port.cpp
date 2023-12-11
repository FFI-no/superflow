// Copyright 2019, Forsvarets forskningsinstitutt. All rights reserved.
#include "superflow/buffered_consumer_port.h"
#include "superflow/policy.h"
#include "superflow/producer_port.h"

#include "gtest/gtest.h"

#include <ciso646>
#include <future>
#include <thread>

using namespace flow;

constexpr auto Blocking = GetMode::Blocking;
constexpr auto Latched = GetMode::Latched;
constexpr auto Single = ConnectPolicy::Single;
constexpr auto Multi = ConnectPolicy::Multi;

TEST(BufferedConsumer, operator_bool)
{
  using Consumer = BufferedConsumerPort<int, Single, Blocking>;
  auto consumer = std::make_shared<Consumer>();

  ASSERT_TRUE(*consumer);
  consumer->deactivate();
  ASSERT_FALSE(*consumer);
}

TEST(BufferedConsumer, deactivate_terminates_buffer)
{
  using Producer = ProducerPort<int>;
  using Consumer = BufferedConsumerPort<int, Single, Blocking>;

  auto producer = std::make_shared<Producer>();
  auto consumer = std::make_shared<Consumer>(10);

  ASSERT_NO_THROW(producer->connect(consumer));
  ASSERT_EQ(1, producer->numConnections());

  ASSERT_NO_THROW(producer->send(42));
  ASSERT_NO_THROW(producer->send(84));
  ASSERT_EQ(2, consumer->getQueueSize());
  ASSERT_EQ(2, producer->getStatus().num_transactions);
  ASSERT_EQ(0, consumer->getStatus().num_transactions);

  ASSERT_EQ(*consumer->getNext(), 42);
  ASSERT_EQ(1, consumer->getQueueSize());
  consumer->deactivate();

  ASSERT_EQ(consumer->getNext(), std::nullopt);
  ASSERT_EQ(1, consumer->getQueueSize());
}

TEST(BufferedConsumer, deactivate_does_not_disconnect_but_does_not_receive_either)
{
  using Producer = ProducerPort<int>;
  using Consumer = BufferedConsumerPort<int, Multi>;

  auto producer = std::make_shared<Producer>();
  auto consumer = std::make_shared<Consumer>();

  ASSERT_EQ(producer.use_count(), 1);
  ASSERT_EQ(consumer.use_count(), 1);

  ASSERT_NO_THROW(consumer->connect(producer));

  ASSERT_EQ(producer.use_count(), 2);
  ASSERT_EQ(consumer.use_count(), 3);
  ASSERT_EQ(1, producer->numConnections());

  ASSERT_NO_THROW(consumer->deactivate());
  ASSERT_TRUE(consumer->isConnected());

  ASSERT_NO_THROW(producer->send(42));
  ASSERT_EQ(0, consumer->getQueueSize());

  ASSERT_EQ(1, producer->numConnections());

  ASSERT_EQ(1, producer->getStatus().num_transactions);
  ASSERT_EQ(0, consumer->getStatus().num_transactions);
}

TEST(BufferedConsumer, receiveData)
{
  using Producer = ProducerPort<int>;
  using Consumer = BufferedConsumerPort<int, Single, Blocking>;

  auto producer = std::make_shared<Producer>();
  auto consumer = std::make_shared<Consumer>();

  ASSERT_NO_THROW(producer->connect(consumer));
  ASSERT_NO_THROW(producer->send(42));
  int item{0};
  ASSERT_NO_THROW(item = consumer->getNext().value());
  ASSERT_EQ(42, item);
}

TEST(BufferedConsumer, receiveInvalidData)
{
  {
    auto consumer = std::make_shared<BufferedConsumerPort<int>>();

    consumer->deactivate();

    ASSERT_THROW(auto item = consumer->getNext().value(), std::bad_optional_access);
    ASSERT_NO_THROW(auto item = consumer->getNext());
    ASSERT_EQ(std::nullopt, consumer->getNext());
    ASSERT_FALSE(consumer->getNext().has_value());

    // This is ok, but undefined behavior
    // const int& cat = *(consumer->getNext());
  }
}

TEST(BufferedConsumer, receiveLatchedData)
{
  using Producer = ProducerPort<int>;

  auto producer = std::make_shared<Producer>();
  auto consumer = std::make_shared<BufferedConsumerPort<int, Single, Latched>>();

  ASSERT_NO_THROW(producer->connect(consumer));
  ASSERT_NO_THROW(producer->send(42));
  int item{0};
  ASSERT_NO_THROW(item = consumer->getNext().value());
  ASSERT_EQ(42, item);
  ASSERT_NO_THROW(item = consumer->getNext().value());
  ASSERT_EQ(42, item);
  ASSERT_NO_THROW(producer->send(43));
  ASSERT_NO_THROW(item = consumer->getNext().value());
  ASSERT_EQ(43, item);
}

TEST(BufferedConsumer, latchedDoesAlsoPop)
{
  using Producer = ProducerPort<int>;

  auto producer = std::make_shared<Producer>();
  auto consumer = std::make_shared<BufferedConsumerPort<int, Single, Latched>>(3);

  ASSERT_NO_THROW(producer->connect(consumer));

  {
    int item{0};
    ASSERT_NO_THROW(producer->send(42));
    ASSERT_NO_THROW(item = consumer->getNext().value());
    ASSERT_EQ(42, item);
  }
  {
    int item{0};
    ASSERT_NO_THROW(item = consumer->getNext().value());
    ASSERT_EQ(42, item);
  }
  {
    int item{0};
    ASSERT_NO_THROW(producer->send(43));
    ASSERT_NO_THROW(item = consumer->getNext().value());
    ASSERT_EQ(43, item);
  }
  {
    int item{0};
    ASSERT_NO_THROW(item = consumer->getNext().value());
    ASSERT_EQ(43, item);
  }
  {
    int item{0};
    ASSERT_NO_THROW(producer->send(44));
    ASSERT_NO_THROW(producer->send(45));
    ASSERT_NO_THROW(producer->send(46));
    ASSERT_NO_THROW(item = consumer->getNext().value());
    ASSERT_EQ(44, item);
  }
}

TEST(BufferedConsumer, latchedConsumerPortCanBeDeactivated)
{
  using Producer = ProducerPort<int>;

  auto producer = std::make_shared<Producer>();
  auto consumer = std::make_shared<BufferedConsumerPort<int, Single, Latched>>();

  ASSERT_NO_THROW(producer->connect(consumer));

  std::atomic_bool was_terminated{false};
  std::thread consumer_thread(
      [&]()
      {
        const auto result = consumer->getNext();
        if (not result)
        { was_terminated = true; }
      });
  consumer->deactivate();
  consumer_thread.join();
  EXPECT_TRUE(was_terminated);
}

TEST(BufferedConsumer, iterate)
{
  auto producer = std::make_shared<ProducerPort<int>>();
  auto consumer = std::make_shared<BufferedConsumerPort<int>>(20);

  producer->connect(consumer);

  std::promise<void> all_sent;

  auto worker = std::async(
      std::launch::async,
      [producer, consumer, &all_sent]()
      {
        for (int i = 0; i < 10; ++i)
        {
          producer->send(i);
        }
        ASSERT_EQ(10, producer->getStatus().num_transactions);
        all_sent.set_value();
      }
  );

  {
    using namespace std::chrono_literals;
    const auto future_status = all_sent.get_future().wait_for(1s);
    ASSERT_NE(future_status, std::future_status::timeout);
  }

  std::vector<int> values;

  size_t transactions{0};
  for (const auto& v : *consumer)
  {
    ASSERT_EQ(++transactions, consumer->getStatus().num_transactions);
    ASSERT_EQ(v, static_cast<int>(values.size()));
    values.push_back(v);

    if (values.size() >= 10)
    { consumer->deactivate(); }
  }
  ASSERT_EQ(10, consumer->getStatus().num_transactions);
  ASSERT_EQ(10, values.size());
}

struct A
{
  A() = delete;
  A(int i) : value{i} {}
  constexpr operator int() const { return value; }
  int value;
};

TEST(BufferedConsumer, consumeObjectsWithNoDefaultConstructor)
{
  auto producer = std::make_shared<ProducerPort<A>>();
  auto consumer = std::make_shared<BufferedConsumerPort<A>>(20);

  producer->connect(consumer);

  std::promise<void> all_sent;

  auto worker = std::async(
      std::launch::async,
      [producer, consumer, &all_sent]()
      {
        for (int i = 0; i < 10; ++i)
        {
          producer->send(i);
        }

        ASSERT_EQ(10, producer->getStatus().num_transactions);
        all_sent.set_value();
      }
  );

  {
    using namespace std::chrono_literals;
    const auto future_status = all_sent.get_future().wait_for(1s);
    ASSERT_NE(future_status, std::future_status::timeout);
  }

  std::vector<int> values;

  for (const auto& v : *consumer)
  {
    ASSERT_EQ(v, static_cast<int>(values.size()));
    values.push_back(v);

    if (values.size() >= 10)
    { consumer->deactivate(); }
  }
  ASSERT_EQ(10, values.size());
}

TEST(BufferedConsumer, streamOperator)
{
  auto consumer = std::make_shared<BufferedConsumerPort<int>>(10);

  consumer->receive(42, nullptr);
  int result;
  EXPECT_TRUE(*consumer >> result);
  EXPECT_EQ(42, result);

  consumer->deactivate();
  EXPECT_FALSE(*consumer >> result);
  EXPECT_EQ(42, result);
}

TEST(BufferedConsumer, blockingHasNext)
{
  BufferedConsumerPort<int, ConnectPolicy::Single, GetMode::Blocking> consumer(1);

  ASSERT_FALSE(consumer.hasNext());

  consumer.receive(0, nullptr);

  ASSERT_TRUE(consumer.hasNext());

  {
    int i = consumer.getNext().value();
  }

  ASSERT_FALSE(consumer.hasNext());
}

TEST(BufferedConsumer, latchedHasNext)
{
  BufferedConsumerPort<int, ConnectPolicy::Single, GetMode::Latched> consumer(1);

  ASSERT_FALSE(consumer.hasNext());

  consumer.receive(0, nullptr);

  ASSERT_TRUE(consumer.hasNext());

  {
    int i = consumer.getNext().value();
  }

  ASSERT_TRUE(consumer.hasNext());
}

TEST(BufferedConsumer, clearBlocking)
{
  BufferedConsumerPort<int, ConnectPolicy::Single, GetMode::Blocking> consumer(1);

  consumer.receive(0, nullptr);

  ASSERT_TRUE(consumer.hasNext());

  consumer.clear();

  ASSERT_FALSE(consumer.hasNext());
}

TEST(BufferedConsumer, clearLatched)
{
  {
    BufferedConsumerPort<int, ConnectPolicy::Single, GetMode::Latched> consumer(1);

    consumer.receive(0, nullptr);

    ASSERT_TRUE(consumer.hasNext());

    consumer.clear();

    ASSERT_FALSE(consumer.hasNext());
  }
  {
    BufferedConsumerPort<int, ConnectPolicy::Single, GetMode::Latched> consumer(1);
    consumer.receive(42, nullptr);
    ASSERT_TRUE(consumer.hasNext());

    int item{0};
    ASSERT_NO_THROW(item = consumer.getNext().value());
    ASSERT_EQ(42, item);

    ASSERT_TRUE(consumer.hasNext());
    consumer.clear();
    ASSERT_FALSE(consumer.hasNext());
  }
}

TEST(BufferedConsumer, conversion)
{
  using Consumer = BufferedConsumerPort<bool, ConnectPolicy::Multi, GetMode::Blocking, LeakPolicy::Leaky, int>;

  const auto bool_producer = std::make_shared<ProducerPort<bool>>();
  const auto int_producer = std::make_shared<ProducerPort<int>>();
  const auto consumer = std::make_shared<Consumer>(1);

  ASSERT_NO_THROW(consumer->connect(bool_producer));
  ASSERT_NO_THROW(consumer->connect(int_producer));

  int_producer->send(2);
  ASSERT_TRUE(consumer->getNext().value_or(false));

  bool_producer->send(true);
  ASSERT_TRUE(consumer->getNext().value_or(false));
}

namespace
{
struct IntClass
{
  std::string name;
  int val;

  operator int() const
  {
    return val;
  }
};
}

TEST(BufferedConsumerPort, implicit_conversion)
{
  using Consumer = BufferedConsumerPort<int, ConnectPolicy::Multi, GetMode::Blocking, LeakPolicy::Leaky, IntClass>;

  const auto producer = std::make_shared<ProducerPort<int>>();
  const auto c_producer = std::make_shared<ProducerPort<IntClass>>();
  const auto consumer = std::make_shared<Consumer>(1);

  ASSERT_NO_THROW(consumer->connect(producer));
  ASSERT_NO_THROW(consumer->connect(c_producer));

  producer->send(2);
  ASSERT_EQ(consumer->getNext().value_or(-1), 2);

  c_producer->send({"hei", 2});
  ASSERT_EQ(consumer->getNext().value_or(-1), 2);
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

TEST(BufferedConsumer, upCast)
{
  using Consumer = BufferedConsumerPort<Base, ConnectPolicy::Single, GetMode::Blocking, LeakPolicy::Leaky, Derived>;
  const auto consumer = std::make_shared<Consumer>(1);

  consumer->receive(Base{}, nullptr);
  ASSERT_TRUE(consumer->getNext().has_value());
  ASSERT_FALSE(consumer->hasNext());

  const auto producer = std::make_shared<ProducerPort<Derived>>();
  ASSERT_NO_THROW(consumer->connect(producer));

  producer->send({});
  ASSERT_TRUE(consumer->getNext().has_value());
  ASSERT_FALSE(consumer->hasNext());
}

TEST(BufferedConsumer, variantReceive)
{
  using Consumer = BufferedConsumerPort<std::variant<int, std::string>>;
  Consumer consumer{1};

  consumer.receive(10, nullptr);

  {
    const auto opt_value = consumer.getNext();
    ASSERT_TRUE(opt_value.has_value());

    const auto& variant = opt_value.value();
    ASSERT_TRUE(std::holds_alternative<int>(variant));
    ASSERT_EQ(std::get<int>(variant), 10);
  }

  ASSERT_FALSE(consumer.hasNext());

  consumer.receive("hei", nullptr);

  {
    const auto opt_value = consumer.getNext();
    ASSERT_TRUE(opt_value.has_value());

    const auto& variant = opt_value.value();
    ASSERT_TRUE(std::holds_alternative<std::string>(variant));
    ASSERT_EQ(std::get<std::string>(variant), "hei");
  }
}

TEST(BufferedConsumer, variantConnect)
{
  using Consumer = BufferedConsumerPort<std::variant<int, std::string>>;
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

TEST(BufferedConsumer, getQueueSize)
{
  using Producer = ProducerPort<int>;
  using Consumer = BufferedConsumerPort<int, Single, Blocking>;

  auto producer = std::make_shared<Producer>();
  auto consumer = std::make_shared<Consumer>(10);

  ASSERT_NO_THROW(producer->connect(consumer));
  ASSERT_EQ(consumer->getQueueSize(), 0);
  ASSERT_NO_THROW(producer->send(42));
  ASSERT_EQ(consumer->getQueueSize(), 1);
  int item{0};
  ASSERT_NO_THROW(item = consumer->getNext().value());
  ASSERT_EQ(consumer->getQueueSize(), 0);
  ASSERT_EQ(42, item);
}

TEST(BufferedConsumer, notLeaky)
{
  using namespace std::chrono_literals;

  using Producer = ProducerPort<int>;
  using LeakyConsumer = BufferedConsumerPort<int, Multi, Blocking, LeakPolicy::Leaky>;
  using BlockConsumer = BufferedConsumerPort<int, Multi, Blocking, LeakPolicy::PushBlocking>;

  auto producer = std::make_shared<Producer>();

  auto leaky_consumer = std::make_shared<LeakyConsumer>(1);
  auto block_consumer = std::make_shared<BlockConsumer>(1);

  ASSERT_NO_THROW(producer->connect(leaky_consumer));
  ASSERT_NO_THROW(producer->send(21));
  ASSERT_NO_THROW(producer->send(7)); // We expect leaky consumer to not block the producer.

  ASSERT_EQ(leaky_consumer->getQueueSize(), 1);
  ASSERT_EQ(block_consumer->getQueueSize(), 0); // Is not connected yet, so, duh...

  EXPECT_EQ(7, leaky_consumer->getNext().value());
  ASSERT_EQ(leaky_consumer->getQueueSize(), 0); // 21 is forever gone

  ASSERT_NO_THROW(producer->connect(block_consumer));

  ASSERT_EQ(leaky_consumer->getQueueSize(), 0);
  ASSERT_EQ(block_consumer->getQueueSize(), 0);

  ASSERT_NO_THROW(producer->send(42));

  using namespace std::chrono_literals;
  std::promise<void> producer_sent;

  auto pusher = std::async(std::launch::async, [&producer, &producer_sent]{
    producer->send(1984);
    producer_sent.set_value();
  });

  ASSERT_EQ(leaky_consumer->getQueueSize(), 1);
  ASSERT_EQ(block_consumer->getQueueSize(), 1);

  const auto future = producer_sent.get_future();

  // producer->send should be stalling now because block_consumer has full buffer
  ASSERT_EQ(future.wait_for(10ms), std::future_status::timeout);

  // EXPECT_EQ(1984, leaky_consumer->getNext().value());
  // might or might not be - depends on who is first in line, leaky or blocky consumer... so, cannot test.

  EXPECT_EQ(42, block_consumer->getNext().value()); // this will unblock the pusher
  pusher.wait();
  ASSERT_NE(future.wait_for(1s), std::future_status::timeout);

  ASSERT_EQ(1984, leaky_consumer->getNext().value()); // 42 is forever gone

  ASSERT_EQ(leaky_consumer->getQueueSize(), 0);
  ASSERT_EQ(block_consumer->getQueueSize(), 1);

  EXPECT_EQ(1984, block_consumer->getNext().value());
  ASSERT_EQ(block_consumer->getQueueSize(), 0);
}
