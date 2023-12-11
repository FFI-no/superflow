// Copyright 2019, Forsvarets forskningsinstitutt. All rights reserved.
#include "superflow/proxel.h"

#include "gtest/gtest.h"

using namespace flow;

class MyProxel : public Proxel
{
public:
  MyProxel() = default;

  explicit MyProxel(const State state)
  {
    setState(state);
  };

  explicit MyProxel(const std::string& status_info)
  {
    setStatusInfo(status_info);
  };

  ~MyProxel() override = default;

  void start() override {};

  void stop() noexcept override {};

  void pushState(const State state)
  {
    setState(state);
  }

  void pushStatusInfo(const std::string& status_info)
  {
    setStatusInfo(status_info);
  }
};

TEST(ProxelStatus, default_state_is_undefined)
{
  MyProxel proxel;
  ASSERT_EQ(proxel.getStatus().state, ProxelStatus::State::Undefined);
}

TEST(ProxelStatus, setState_works)
{
  MyProxel proxel;

  proxel.pushState(ProxelStatus::State::Running);
  ASSERT_EQ(proxel.getStatus().state, ProxelStatus::State::Running);
}

TEST(ProxelStatus, setState_works_from_ctor)
{
  MyProxel proxel{ProxelStatus::State::Paused};
  ASSERT_EQ(proxel.getStatus().state, ProxelStatus::State::Paused);

  proxel.pushState(ProxelStatus::State::Running);
  ASSERT_EQ(proxel.getStatus().state, ProxelStatus::State::Running);
}

TEST(ProxelStatus, default_status_info_is_empty)
{
  MyProxel proxel;
  ASSERT_EQ(proxel.getStatus().info, "");
}

TEST(ProxelStatus, setStatusInfo_works)
{
  MyProxel proxel;

  const std::string info = "hallo";
  proxel.pushStatusInfo(info);
  ASSERT_EQ(proxel.getStatus().info, info);
}

TEST(ProxelStatus, setStatusInfo_works_from_ctor)
{
  const std::string orig_info = "hallo";
  MyProxel proxel{orig_info};
  ASSERT_EQ(proxel.getStatus().info, orig_info);

  proxel.pushState(ProxelStatus::State::Running);
  const std::string other_info = "heihei";
  proxel.pushStatusInfo(other_info);
  ASSERT_EQ(proxel.getStatus().info, other_info);
}
