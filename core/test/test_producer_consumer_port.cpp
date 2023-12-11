// Copyright 2019, Forsvarets forskningsinstitutt. All rights reserved.
#include "superflow/buffered_consumer_port.h"
#include "superflow/policy.h"
#include "superflow/producer_port.h"

#include "gtest/gtest.h"

using namespace flow;
constexpr auto Single = ConnectPolicy::Single;

TEST(ProducerConsumer, connectNoThrow)
{
  using Producer = ProducerPort<int>;
  using Consumer = BufferedConsumerPort<int, Single>;

  auto producer = std::make_shared<Producer>();
  auto consumer = std::make_shared<Consumer>();

  ASSERT_NO_THROW(producer->connect(consumer));
}

TEST(Producer, disconnectNoThrow)
{
  using Producer = ProducerPort<int>;
  using Consumer = BufferedConsumerPort<int, Single>;

  auto producer = std::make_shared<Producer>();
  auto consumer = std::make_shared<Consumer>();

  ASSERT_NO_THROW(producer->disconnect());
  ASSERT_NO_THROW(producer->disconnect(consumer));
}

TEST(Consumer, disconnectNoThrow)
{
  using Consumer = BufferedConsumerPort<int, Single>;

  auto consumer = std::make_shared<Consumer>();

  ASSERT_NO_THROW(consumer->disconnect());
}

TEST(ProducerConsumer, connectMismatchThrows)
{
  using Producer = ProducerPort<int>;
  using Consumer = BufferedConsumerPort<bool, Single>;

  auto producer = std::make_shared<Producer>();
  auto consumer = std::make_shared<Consumer>();

  ASSERT_THROW(producer->connect(consumer), std::invalid_argument);
}

TEST(ProducerConsumer, connectWorksBothWays)
{
  using Producer = ProducerPort<int>;
  using Consumer = BufferedConsumerPort<int, Single>;

  {
    auto producer = std::make_shared<Producer>();
    auto consumer = std::make_shared<Consumer>();

    ASSERT_NO_THROW(producer->connect(consumer));

    ASSERT_EQ(producer->numConnections(), 1);
    ASSERT_TRUE(consumer->isConnected());
  }

  {
    auto producer = std::make_shared<Producer>();
    auto consumer = std::make_shared<Consumer>();

    ASSERT_NO_THROW(consumer->connect(producer));

    ASSERT_EQ(producer->numConnections(), 1);
    ASSERT_TRUE(consumer->isConnected());
  }
}

TEST(ProducerConsumer, disconnectWorksBothWays)
{
  using Producer = ProducerPort<int>;
  using Consumer = BufferedConsumerPort<int, Single>;

  {
    auto producer = std::make_shared<Producer>();
    auto consumer = std::make_shared<Consumer>();

    ASSERT_NO_THROW(producer->connect(consumer));

    ASSERT_EQ(producer->numConnections(), 1);
    ASSERT_TRUE(consumer->isConnected());

    producer->disconnect();

    ASSERT_EQ(producer->numConnections(), 0);
    ASSERT_FALSE(consumer->isConnected());
  }

  {
    auto producer = std::make_shared<Producer>();
    auto consumer = std::make_shared<Consumer>();

    ASSERT_NO_THROW(consumer->connect(producer));

    ASSERT_EQ(producer->numConnections(), 1);
    ASSERT_TRUE(consumer->isConnected());

    consumer->disconnect();

    ASSERT_EQ(producer->numConnections(), 0);
    ASSERT_FALSE(consumer->isConnected());
  }
}

TEST(ProducerConsumer, newConnectToProducerAddsConnection)
{
  using Producer = ProducerPort<int>;
  using Consumer = BufferedConsumerPort<int, Single>;

  auto producer = std::make_shared<Producer>();
  auto consumer1 = std::make_shared<Consumer>();
  auto consumer2 = std::make_shared<Consumer>();

  producer->connect(consumer1);

  ASSERT_EQ(producer->numConnections(), 1);
  ASSERT_TRUE(consumer1->isConnected());

  producer->connect(consumer2);

  ASSERT_EQ(producer->numConnections(), 2);
  ASSERT_TRUE(consumer1->isConnected());
  ASSERT_TRUE(consumer2->isConnected());
}

TEST(ProducerConsumer, newConnectToConsumerThrows)
{
  using Producer = ProducerPort<int>;
  using Consumer = BufferedConsumerPort<int, Single>;

  auto consumer = std::make_shared<Consumer>();
  auto producer1 = std::make_shared<Producer>();
  auto producer2 = std::make_shared<Producer>();

  consumer->connect(producer1);

  ASSERT_TRUE(consumer->isConnected());
  ASSERT_EQ(producer1->numConnections(), 1);
  ASSERT_EQ(producer2->numConnections(), 0);

  ASSERT_THROW(consumer->connect(producer2), std::invalid_argument);
}

TEST(ProducerConsumer, multipleConnectOfSamePortNoThrow)
{
  using Producer = ProducerPort<int>;
  using Consumer = BufferedConsumerPort<int, Single>;

  auto producer = std::make_shared<Producer>();
  auto consumer = std::make_shared<Consumer>();

  ASSERT_NO_FATAL_FAILURE(producer->connect(consumer));
  ASSERT_NO_FATAL_FAILURE(producer->connect(consumer));
  ASSERT_NO_FATAL_FAILURE(consumer->connect(producer));

  ASSERT_EQ(producer->numConnections(), 1);
  ASSERT_TRUE(consumer->isConnected());
}

TEST(ProducerConsumer, pointersAreFreedOnDisconnect)
{
  using Producer = ProducerPort<int>;
  using Consumer = BufferedConsumerPort<int, Single>;

  {
    auto producer = std::make_shared<Producer>();
    auto consumer = std::make_shared<Consumer>();

    ASSERT_EQ(producer.use_count(), 1);
    ASSERT_EQ(consumer.use_count(), 1);

    producer->connect(consumer);

    ASSERT_EQ(producer.use_count(), 2);
    ASSERT_EQ(consumer.use_count(), 3);

    producer->disconnect();

    ASSERT_EQ(producer.use_count(), 1);
    ASSERT_EQ(consumer.use_count(), 1);
  }

  {
    auto producer = std::make_shared<Producer>();
    auto consumer = std::make_shared<Consumer>();

    ASSERT_EQ(producer.use_count(), 1);
    ASSERT_EQ(consumer.use_count(), 1);

    consumer->connect(producer);

    ASSERT_EQ(producer.use_count(), 2);
    ASSERT_EQ(consumer.use_count(), 3);

    consumer->disconnect();

    ASSERT_EQ(producer.use_count(), 1);
    ASSERT_EQ(consumer.use_count(), 1);
  }
}

TEST(ProducerConsumer, numTransactions)
{
  using Producer = ProducerPort<int>;
  using Consumer = BufferedConsumerPort<int, Single, GetMode::Blocking>;

  auto producer = std::make_shared<Producer>();
  auto consumer = std::make_shared<Consumer>();

  ASSERT_NO_THROW(producer->connect(consumer));

  EXPECT_EQ(0, producer->getStatus().num_transactions);
  EXPECT_EQ(0, consumer->getStatus().num_transactions);

  ASSERT_NO_THROW(producer->send(42));

  EXPECT_EQ(1, producer->getStatus().num_transactions);
  EXPECT_EQ(0, consumer->getStatus().num_transactions);

  ASSERT_NO_FATAL_FAILURE( std::ignore = consumer->getNext().value());
  EXPECT_EQ(1, consumer->getStatus().num_transactions);
}

TEST(Producer, specificDisconnect)
{
  using Producer = ProducerPort<int>;
  using Consumer = BufferedConsumerPort<int, Single>;

  auto producer = std::make_shared<Producer>();
  auto consumer1 = std::make_shared<Consumer>();
  auto consumer2 = std::make_shared<Consumer>();

  producer->connect(consumer1);
  producer->connect(consumer2);

  ASSERT_EQ(producer->numConnections(), 2);
  ASSERT_TRUE(consumer1->isConnected());
  ASSERT_TRUE(consumer2->isConnected());

  producer->disconnect(consumer1);

  ASSERT_EQ(producer->numConnections(), 1);
  ASSERT_FALSE(consumer1->isConnected());
  ASSERT_TRUE(consumer2->isConnected());

  producer->disconnect(consumer2);
  ASSERT_EQ(producer->numConnections(), 0);
  ASSERT_FALSE(consumer1->isConnected());
  ASSERT_FALSE(consumer2->isConnected());
}

TEST(Producer, generalDisconnect)
{
  using Producer = ProducerPort<int>;
  using Consumer = BufferedConsumerPort<int, Single>;

  auto producer = std::make_shared<Producer>();

  constexpr size_t num_consumers = 10;
  std::vector<std::shared_ptr<Consumer>> consumers;

  for (size_t i = 0; i < num_consumers; ++i)
  {
    auto consumer = std::make_shared<Consumer>();
    producer->connect(consumer);

    ASSERT_TRUE(consumer->isConnected());

    consumers.push_back(std::move(consumer));
  }

  ASSERT_EQ(producer->numConnections(), num_consumers);

  producer->disconnect();

  ASSERT_EQ(producer->numConnections(), 0);

  for (const auto& consumer : consumers)
  {
    ASSERT_FALSE(consumer->isConnected());
  }
}

TEST(Producer, conversion)
{
  using Producer = ProducerPort<int, bool>;

  const auto producer = std::make_shared<Producer>();
  const auto bool_consumer = std::make_shared<BufferedConsumerPort<bool>>(1);
  const auto int_consumer = std::make_shared<BufferedConsumerPort<int>>(1);

  ASSERT_NO_THROW(bool_consumer->connect(producer));
  ASSERT_NO_THROW(int_consumer->connect(producer));

  producer->send(2);
  ASSERT_TRUE(bool_consumer->getNext().value_or(false));
  ASSERT_EQ(int_consumer->getNext().value_or(-1), 2);
}

namespace
{
struct IntClass
{
  std::string name;
  int val;

  operator int() const // NOLINT intentionally implicit
  {
    return val;
  }

  explicit operator bool() const
  {
    return val;
  }
};
}

TEST(Producer, implicit_conversion)
{
  using Producer = ProducerPort<IntClass, int>;

  const auto producer = std::make_shared<Producer>();
  const auto consumer = std::make_shared<BufferedConsumerPort<IntClass>>(1);
  const auto int_consumer = std::make_shared<BufferedConsumerPort<int>>(1);

  ASSERT_NO_THROW(consumer->connect(producer));
  ASSERT_NO_THROW(int_consumer->connect(producer));

  producer->send({"hei", 2});
  ASSERT_EQ(int_consumer->getNext().value_or(-1), 2);
  ASSERT_EQ(consumer->getNext().value(), 2);
}

TEST(Producer, explicit_conversion)
{
  using Producer = ProducerPort<IntClass, bool>;

  const auto producer = std::make_shared<Producer>();
  const auto consumer = std::make_shared<BufferedConsumerPort<IntClass>>(1);
  const auto bool_consumer = std::make_shared<BufferedConsumerPort<bool>>(1);

  ASSERT_NO_THROW(consumer->connect(producer));
  ASSERT_NO_THROW(bool_consumer->connect(producer));

  producer->send({"hei", 10});
  ASSERT_EQ(consumer->getNext().value().val, 10);
  ASSERT_EQ(bool_consumer->getNext().value_or(false), true);
}

namespace
{
struct CIntClass
{
  CIntClass(
    const std::string& name_in,
    const int val_in
  )
    : name{name_in}
    , val{val_in}
  {}

  CIntClass(
    const std::string& name_in
  )
    : name{name_in}
    , val{0}
  {}

  explicit CIntClass(
    const IntClass& int_class
  )
    : name{int_class.name}
    , val{int_class.val}
  {}

  std::string name;
  int val;
};
}

TEST(Producer, implicit_ctor_conversion)
{
  const auto cint_producer = std::make_shared<ProducerPort<CIntClass>>();
  const auto string_producer = std::make_shared<ProducerPort<std::string>>();
  const auto consumer = std::make_shared<BufferedConsumerPort<CIntClass, ConnectPolicy::Multi, GetMode::Blocking, LeakPolicy::Leaky, std::string>>(1);

  ASSERT_NO_THROW(consumer->connect(cint_producer));
  ASSERT_NO_THROW(string_producer->connect(consumer));

  {
    const CIntClass v = {"hei", 2};
    cint_producer->send(v);

    const auto res = consumer->getNext().value();
    ASSERT_EQ(res.name, v.name);
    ASSERT_EQ(res.val, v.val);
  }

  {
    const std::string v = "hei";
    string_producer->send(v);

    const auto res = consumer->getNext().value();
    ASSERT_EQ(res.name, v);
    ASSERT_EQ(res.val, 0);
  }
}

TEST(Producer, explicit_ctor_conversion)
{
  const auto cint_producer = std::make_shared<ProducerPort<CIntClass>>();
  const auto int_producer = std::make_shared<ProducerPort<IntClass>>();
  const auto consumer = std::make_shared<BufferedConsumerPort<CIntClass, ConnectPolicy::Multi, GetMode::Blocking, LeakPolicy::Leaky, IntClass>>(1);

  ASSERT_NO_THROW(consumer->connect(int_producer));
  ASSERT_NO_THROW(cint_producer->connect(consumer));

  {
    const CIntClass v = {"hei", 2};
    cint_producer->send(v);

    const auto res = consumer->getNext().value();
    ASSERT_EQ(res.name, v.name);
    ASSERT_EQ(res.val, v.val);
  }

  {
    const IntClass v = {"heia", 3};
    int_producer->send(v);

    const auto res = consumer->getNext().value();
    ASSERT_EQ(res.name, v.name);
    ASSERT_EQ(res.val, v.val);
  }
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

TEST(Producer, upCast)
{
  using Producer = ProducerPort<Derived, Base>;

  const auto producer = std::make_shared<Producer>();
  const auto base_consumer = std::make_shared<BufferedConsumerPort<Base>>(1);
  const auto derived_consumer = std::make_shared<BufferedConsumerPort<Derived>>(1);

  ASSERT_NO_THROW(base_consumer->connect(producer));
  ASSERT_NO_THROW(derived_consumer->connect(producer));

  producer->send(Derived{});
  ASSERT_TRUE(base_consumer->hasNext());
  ASSERT_TRUE(derived_consumer->hasNext());
}

struct Ax { size_t a; };
struct Bx : public Ax { float f; };
struct Cx { std::string str; };

TEST(Producer, incompatible_types)
{
  using Producer = ProducerPort<Bx, Ax>;
  using Consumer_B = BufferedConsumerPort<Ax, ConnectPolicy::Single, GetMode::Blocking>;
  using Consumer_C = BufferedConsumerPort<Cx, ConnectPolicy::Single, GetMode::Blocking>;

  const auto producer = std::make_shared<Producer>();
  const auto b_consum = std::make_shared<Consumer_B>(1);
  const auto c_consum = std::make_shared<Consumer_C>(1);

  ASSERT_NO_THROW(producer->connect(b_consum));
  ASSERT_THROW(producer->connect(c_consum), std::invalid_argument);

  const Bx bx{{42}, 2.f};
  producer->send(bx);

  ASSERT_TRUE(b_consum->hasNext());
  ASSERT_EQ(b_consum->getNext().value().a, 42);
}
