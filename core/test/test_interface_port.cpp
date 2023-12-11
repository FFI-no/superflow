// Copyright 2019, Forsvarets forskningsinstitutt. All rights reserved.
#include "superflow/interface_port.h"

#include "gtest/gtest.h"

using namespace flow;

class MyInterface
{
public:
  virtual int foo(int bar) const = 0;
};

class MyHost : public MyInterface
{
public:
  MyHost()
    : port{std::make_shared<InterfacePort<MyInterface>::Host>(*this)}
  {}

  int foo(const int bar) const override
  {
    return 2*bar;
  }

  InterfacePort<MyInterface>::Host::Ptr port;
};

TEST(InterfacePort, happy_path)
{
  using Client = InterfacePort<MyInterface>::Client;

  MyHost host;
  const auto client = std::make_shared<Client>();

  client->connect(host.port);

  ASSERT_EQ(2, client->get().foo(1));
  ASSERT_EQ(42, client->get().foo(21));
}

TEST(InterfacePort, isConnected)
{
  using Client = InterfacePort<MyInterface>::Client;

  MyHost host;
  const auto client = std::make_shared<Client>();

  EXPECT_FALSE(host.port->isConnected());
  EXPECT_FALSE(client->isConnected());

  client->connect(host.port);

  EXPECT_TRUE(host.port->isConnected());
  EXPECT_TRUE(client->isConnected());
}

TEST(InterfacePort, getThrowsIfNotConnected)
{
  using Client = InterfacePort<MyInterface>::Client;

  MyHost host;
  const auto client = std::make_shared<Client>();

  ASSERT_FALSE(host.port->isConnected());
  ASSERT_FALSE(client->isConnected());

  EXPECT_THROW(host.port->get(), std::runtime_error);
  EXPECT_THROW(client->get(), std::runtime_error);

  client->connect(host.port);

  EXPECT_NO_THROW(host.port->get());
  EXPECT_NO_THROW(client->get());
}

TEST(InterfacePort, num_transactions)
{
  using Client = InterfacePort<MyInterface>::Client;

  MyHost my_host;
  const auto& host = my_host.port;
  const auto client = std::make_shared<Client>();

  ASSERT_NO_FATAL_FAILURE(client->connect(host));
  ASSERT_EQ(0, host->getStatus().num_transactions);
  ASSERT_EQ(0, client->getStatus().num_transactions);

  ASSERT_EQ(2, client->get().foo(1));

  EXPECT_EQ(1, host->getStatus().num_transactions);
  EXPECT_EQ(1, client->getStatus().num_transactions);

  ASSERT_NO_FATAL_FAILURE( client->get());

  EXPECT_EQ(2, host->getStatus().num_transactions);
  EXPECT_EQ(2, client->getStatus().num_transactions);

  client->disconnect();
  EXPECT_THROW(client->get(), std::runtime_error);
  EXPECT_EQ(3, client->getStatus().num_transactions);
  EXPECT_EQ(2, host->getStatus().num_transactions);

  EXPECT_THROW(host->get(), std::runtime_error);
  EXPECT_EQ(3, client->getStatus().num_transactions);
  EXPECT_EQ(3, host->getStatus().num_transactions);
}