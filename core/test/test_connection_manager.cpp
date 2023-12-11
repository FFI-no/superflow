// Copyright 2019, Forsvarets forskningsinstitutt. All rights reserved.
#include "connectable_port.h"
#include "multi_connectable_port.h"

#include "superflow/connection_manager.h"
#include "superflow/policy.h"

#include "gtest/gtest.h"

using namespace flow;
constexpr auto Multi = ConnectPolicy::Multi;
constexpr auto Single = ConnectPolicy::Single;

TEST(ConnectionManager, connectSingle)
{
  auto lhs = std::make_shared<ConnectablePort<int>>();
  auto rhs = std::make_shared<ConnectablePort<int>>();

  ConnectionManager<Single> connection_manager;

  ASSERT_NO_THROW(connection_manager.connect(lhs, rhs));
  ASSERT_TRUE(connection_manager.isConnected());
  ASSERT_TRUE(lhs->isConnected());
  ASSERT_TRUE(rhs->isConnected());
  ASSERT_EQ(lhs->connection_, rhs);
  ASSERT_EQ(rhs->connection_, lhs);
}

TEST(ConnectionManager, mismatchThrowsSingle)
{
  auto lhs = std::make_shared<ConnectablePort<int>>();
  auto rhs = std::make_shared<ConnectablePort<std::string>>();

  ConnectionManager<Single> connection_manager;

  ASSERT_THROW(connection_manager.connect(lhs, rhs), std::invalid_argument);
  ASSERT_FALSE(connection_manager.isConnected());
  ASSERT_FALSE(lhs->isConnected());
  ASSERT_FALSE(rhs->isConnected());
}

TEST(ConnectionManager, disconnectSingle)
{
  auto lhs = std::make_shared<ConnectablePort<int>>();
  auto rhs = std::make_shared<ConnectablePort<int>>();

  ConnectionManager<Single> connection_manager;

  ASSERT_NO_THROW(connection_manager.connect(lhs, rhs));
  ASSERT_NO_THROW(connection_manager.disconnect(lhs));
  ASSERT_FALSE(connection_manager.isConnected());
  ASSERT_FALSE(lhs->isConnected());
  ASSERT_FALSE(rhs->isConnected());
}


TEST(ConnectionManager, connectAfterdisconnectSingle)
{
  auto lhs = std::make_shared<ConnectablePort<int>>();
  auto rhs = std::make_shared<ConnectablePort<int>>();

  ConnectionManager<Single> connection_manager;

  ASSERT_NO_THROW(connection_manager.connect(lhs, rhs));
  ASSERT_EQ(1, connection_manager.getNumConnections());
  ASSERT_NO_THROW(connection_manager.disconnect(lhs));
  ASSERT_EQ(0, connection_manager.getNumConnections());

  ASSERT_FALSE(connection_manager.isConnected());
  ASSERT_FALSE(lhs->isConnected());
  ASSERT_FALSE(rhs->isConnected());
  EXPECT_NO_THROW(connection_manager.connect(lhs, rhs));
  ASSERT_TRUE(connection_manager.isConnected());
}

TEST(ConnectionManager, specificDisconnectSingle)
{
  auto lhs = std::make_shared<ConnectablePort<int>>();
  auto rhs = std::make_shared<ConnectablePort<int>>();

  ConnectionManager<Single> connection_manager;

  ASSERT_NO_THROW(connection_manager.connect(lhs, rhs));
  ASSERT_NO_THROW(connection_manager.disconnect(lhs, rhs));
  ASSERT_FALSE(connection_manager.isConnected());
  ASSERT_FALSE(lhs->isConnected());
  ASSERT_FALSE(rhs->isConnected());
}

TEST(ConnectionManager, emptyDisconnectNoThrowSingle)
{
  auto lhs = std::make_shared<ConnectablePort<int>>();
  auto rhs = std::make_shared<ConnectablePort<int>>();

  ConnectionManager<Single> connection_manager;

  ASSERT_NO_THROW(connection_manager.disconnect(lhs, rhs));
  ASSERT_NO_THROW(connection_manager.disconnect(lhs));
  ASSERT_FALSE(connection_manager.isConnected());
  ASSERT_FALSE(lhs->isConnected());
  ASSERT_FALSE(rhs->isConnected());
}

TEST(ConnectionManager, newConnectThrowsSingle)
{
  auto lhs = std::make_shared<ConnectablePort<int>>();
  auto rhs1 = std::make_shared<ConnectablePort<int>>();
  auto rhs2 = std::make_shared<ConnectablePort<int>>();

  ConnectionManager<Single> connection_manager;

  ASSERT_NO_THROW(connection_manager.connect(lhs, rhs1));
  ASSERT_TRUE(connection_manager.isConnected());
  ASSERT_TRUE(lhs->isConnected());
  ASSERT_TRUE(rhs1->isConnected());
  ASSERT_FALSE(rhs2->isConnected());

  ASSERT_THROW(connection_manager.connect(lhs, rhs2), std::invalid_argument);
}

TEST(ConnectionManager, connectMulti)
{
  constexpr size_t num_ports = 10;

  auto lhs = std::make_shared<MultiConnectablePort<int>>();
  std::vector<std::shared_ptr<MultiConnectablePort<int>>> rhss;

  for (size_t i = 0; i < num_ports; ++i)
  {
    rhss.push_back(std::make_shared<MultiConnectablePort<int>>());
  }

  ConnectionManager<Multi> connection_manager;

  for (size_t i = 0; i < num_ports; ++i)
  {
    const auto& rhs = rhss[i];

    ASSERT_NO_THROW(connection_manager.connect(lhs, rhs));
    ASSERT_TRUE(rhs->isConnected());
    ASSERT_EQ(rhs->getNumConnections(), 1);
  }

  ASSERT_EQ(connection_manager.getNumConnections(), num_ports);
  ASSERT_EQ(lhs->getNumConnections(), num_ports);
}

TEST(ConnectionManager, mismatchThrowsMulti)
{
  constexpr size_t num_ports = 10;

  auto lhs = std::make_shared<MultiConnectablePort<int>>();
  std::vector<std::shared_ptr<MultiConnectablePort<int>>> rhss;

  for (size_t i = 0; i < num_ports; ++i)
  {
    rhss.push_back(std::make_shared<MultiConnectablePort<int>>());
  }

  ConnectionManager<Multi> connection_manager;

  for (size_t i = 0; i < num_ports; ++i)
  {
    const auto& rhs = rhss[i];

    ASSERT_NO_THROW(connection_manager.connect(lhs, rhs));
    ASSERT_TRUE(rhs->isConnected());
    ASSERT_EQ(rhs->getNumConnections(), 1);
  }

  ASSERT_EQ(connection_manager.getNumConnections(), num_ports);
  ASSERT_EQ(lhs->getNumConnections(), num_ports);

  auto mismatch_port = std::make_shared<MultiConnectablePort<std::string>>();
  ASSERT_THROW(connection_manager.connect(lhs, mismatch_port), std::invalid_argument);
  ASSERT_EQ(connection_manager.getNumConnections(), num_ports);
  ASSERT_EQ(lhs->getNumConnections(), num_ports);
  ASSERT_EQ(mismatch_port->getNumConnections(), 0);
  ASSERT_FALSE(mismatch_port->isConnected());
}

TEST(ConnectionManager, disconnectAllMulti)
{
  constexpr size_t num_ports = 10;

  auto lhs = std::make_shared<MultiConnectablePort<int>>();
  std::vector<std::shared_ptr<MultiConnectablePort<int>>> rhss;

  for (size_t i = 0; i < num_ports; ++i)
  {
    rhss.push_back(std::make_shared<MultiConnectablePort<int>>());
  }

  ConnectionManager<Multi> connection_manager;

  for (size_t i = 0; i < num_ports; ++i)
  {
    const auto& rhs = rhss[i];

    connection_manager.connect(lhs, rhs);
  }

  ASSERT_EQ(connection_manager.getNumConnections(), num_ports);
  ASSERT_EQ(lhs->getNumConnections(), num_ports);

  ASSERT_NO_THROW(connection_manager.disconnect(lhs));

  ASSERT_EQ(connection_manager.getNumConnections(), 0);
  ASSERT_EQ(lhs->getNumConnections(), 0);

  for (const auto& rhs : rhss)
  {
    ASSERT_EQ(rhs->getNumConnections(), 0);
    ASSERT_FALSE(rhs->isConnected());
  }
}

TEST(ConnectionManager, disconnectSpecificMulti)
{
  constexpr size_t num_ports = 10;

  auto lhs = std::make_shared<MultiConnectablePort<int>>();
  std::vector<std::shared_ptr<MultiConnectablePort<int>>> rhss;

  for (size_t i = 0; i < num_ports; ++i)
  {
    rhss.push_back(std::make_shared<MultiConnectablePort<int>>());
  }

  ConnectionManager<Multi> connection_manager;

  for (size_t i = 0; i < num_ports; ++i)
  {
    const auto& rhs = rhss[i];

    connection_manager.connect(lhs, rhs);
  }

  ASSERT_EQ(connection_manager.getNumConnections(), num_ports);
  ASSERT_EQ(lhs->getNumConnections(), num_ports);

  const auto& some_rhs = rhss.back();
  ASSERT_FALSE(some_rhs->did_get_disconnect);

  ASSERT_NO_THROW(connection_manager.disconnect(lhs, some_rhs));

  ASSERT_EQ(connection_manager.getNumConnections(), num_ports - 1);
  ASSERT_EQ(lhs->getNumConnections(), num_ports - 1);
  ASSERT_EQ(some_rhs->getNumConnections(), 0);
  ASSERT_FALSE(some_rhs->isConnected());
  ASSERT_TRUE(some_rhs->did_get_disconnect);
}

TEST(ConnectionManager, emptyDisconnectNoThrowMulti)
{
  auto lhs = std::make_shared<MultiConnectablePort<int>>();

  ConnectionManager<Multi> connection_manager;

  ASSERT_NO_THROW(connection_manager.disconnect(lhs));
}
