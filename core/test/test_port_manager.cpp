// Copyright 2019, Forsvarets forskningsinstitutt. All rights reserved.
#include "superflow/port_manager.h"
#include "superflow/requester_port.h"

#include "connectable_port.h"

#include "gtest/gtest.h"

using namespace flow;

TEST(PortManager, throwsIfPortDoesNotExistWhenEmpty)
{
  PortManager manager;
  Port::Ptr result;
  ASSERT_THROW(result = manager.get("does not exist"), std::invalid_argument);
}

TEST(PortManager, throwsIfPortDoesNotExistWhenNonEmpty)
{
  PortManager manager{{
                          {"foo", nullptr}
                      }};

  Port::Ptr result;
  ASSERT_THROW(result = manager.get("does not exist"), std::invalid_argument);
}

TEST(PortManager, returnsCorrectPort)
{
  Port::Ptr some_port = std::make_shared<RequesterPort<int(void)>>();
  Port::Ptr some_other_port = std::make_shared<RequesterPort<int(void)>>();

  PortManager manager{{
                          {"foo", some_port},
                          {"bar", some_other_port},
                          {"baz", std::make_shared<RequesterPort<bool(void)>>()}
                      }};

  Port::Ptr result;
  ASSERT_THROW(result = manager.get("does not exist"), std::invalid_argument);
  ASSERT_FALSE(some_port == some_other_port);
  ASSERT_EQ(manager.get("foo"), some_port);
  ASSERT_EQ(manager.get("bar"), some_other_port);
}

TEST(PortManager, dtorDisconnectsPorts)
{
  constexpr size_t num_ports = 10;
  std::map<std::string, std::shared_ptr<ConnectablePort<int>>> ports;

  for (size_t i = 0; i < num_ports; ++i)
  {
    std::ostringstream ss;
    ss << "port_" << i;

    const std::string port_id = ss.str();

    ports[port_id] = std::make_shared<ConnectablePort<int>>();
    ASSERT_FALSE(ports[port_id]->did_get_disconnect);
  }

  {
    PortManager manager{{ports.begin(), ports.end()}};

    for (const auto& kv : ports)
    {
      ASSERT_FALSE(kv.second->did_get_disconnect);
    }
  }

  for (const auto& kv : ports)
  {
    ASSERT_TRUE(kv.second->did_get_disconnect);
  }
}

TEST(PortManager, getStatusHandlesNullptr)
{
  PortManager manager{{{}}};
  ASSERT_EQ(manager.getPorts().size(), 1);

  const auto& port_ptr = manager.get("");
  ASSERT_EQ(port_ptr, nullptr);

  ASSERT_NO_FATAL_FAILURE(std::ignore = manager.getStatus());

  const auto statuses = manager.getStatus();
  ASSERT_EQ(statuses.size(), 0);
}

