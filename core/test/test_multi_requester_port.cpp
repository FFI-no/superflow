// Copyright 2019, Forsvarets forskningsinstitutt. All rights reserved.
#include "superflow/multi_requester_port.h"
#include "superflow/responder_port.h"

#include "gtest/gtest.h"

#include <thread>

using namespace flow;

TEST(MultiRequester, request)
{
  using Responder = ResponderPort<int(int)>;

  constexpr size_t num_responders = 10;
  auto requester = std::make_shared<MultiRequesterPort<int(int)>>();

  std::vector<std::shared_ptr<Responder>> responders;

  for (size_t i = 0; i < num_responders; ++i)
  {
    responders.push_back(std::make_shared<Responder>(
      [](const int v)
      { return 2 * v; })
    );
  }

  for (auto& responder : responders)
  {
    ASSERT_NO_THROW(responder->connect(requester));
  }

  constexpr int query = 23;
  const auto responses = requester->request(query);

  for (const auto& response : responses)
  {
    ASSERT_EQ(response, 2 * query);
  }
}

TEST(MultiRequester, typeMismatchThrows)
{
  auto requester = std::make_shared<MultiRequesterPort<int(int)>>();
  auto responder = std::make_shared<ResponderPort<int(std::string)>>(
    [](const std::string&)
    { return 0; }
  );

  ASSERT_THROW(requester->connect(responder), std::invalid_argument);
  ASSERT_THROW(responder->connect(requester), std::invalid_argument);
}

TEST(MultiRequester, emptyRequesterReceivesNothing)
{
  auto requester = std::make_shared<MultiRequesterPort<int(int)>>();

  std::vector<int> responses;
  ASSERT_NO_THROW(responses = requester->request(1));
  ASSERT_TRUE(responses.empty());
}

TEST(MultiRequester, responseOrderIsConserved)
{
  using Responder = ResponderPort<int(int)>;

  constexpr size_t num_responders = 10;
  auto requester = std::make_shared<MultiRequesterPort<int(int)>>();

  std::vector<std::shared_ptr<Responder>> responders;

  for (size_t i = 0; i < num_responders; ++i)
  {
    responders.push_back(std::make_shared<Responder>(
      [i](const int v)
      {
        return static_cast<int>((i + 1) * v);
      })
    );
  }

  for (auto& responder : responders)
  {
    ASSERT_NO_THROW(responder->connect(requester));
  }

  constexpr int query = 23;
  const auto first_responses = requester->request(query);

  const auto second_responses = requester->request(2 * query);

  for (size_t i = 0; i < num_responders; ++i)
  {
    ASSERT_EQ(2 * first_responses[i], second_responses[i]);
  }
}

template <typename F>
bool is_ready(const std::future<F>& fu)
{ return fu.valid() && fu.wait_for(std::chrono::seconds(0)) == std::future_status::ready; }

TEST(MultiRequester, async_request)
{
  using Requester = MultiRequesterPort<int(const std::string&)>;
  using Responder = ResponderPort<int(const std::string&)>;

  constexpr size_t num_responders{5};
  auto requester = std::make_shared<Requester>();

  std::vector<std::shared_ptr<Responder>> responders;
  for (size_t i = 0; i < num_responders; ++i)
  {
    responders.push_back(
        std::make_shared<Responder>([i](const std::string& str)
        {
          std::this_thread::sleep_for(std::chrono::milliseconds(100));
          return static_cast<int>((i + 1) * str.size());
        })
    );
  }

  for (auto& responder : responders)
  { ASSERT_NO_THROW(responder->connect(requester)); }

  const std::string str{"42"};
  auto futures = requester->requestAsync(str);
  EXPECT_EQ(futures.size(), num_responders);

  for (const auto& fu : futures)
  {
    EXPECT_FALSE(is_ready(fu));

    if (fu.valid())
    {
      const auto status = fu.wait_for(std::chrono::seconds(0));
      EXPECT_EQ(status, std::future_status::timeout);
    }
  }

  for (size_t i = 0; i < futures.size(); ++i)
  {
    EXPECT_EQ(futures[i].get(), (i+1) * str.size());
  }
}

TEST(MultiRequester, void)
{
  using Responder = ResponderPort<void()>;

  constexpr size_t num_responders = 10;
  auto requester = std::make_shared<MultiRequesterPort<void()>>();

  std::vector<std::shared_ptr<Responder>> responders;
  responders.reserve(num_responders);

  for (size_t i = 0; i < num_responders; ++i)
  {
    responders.push_back(std::make_shared<Responder>([](){}));
  }

  for (auto& responder : responders)
  {
    ASSERT_NO_THROW(responder->connect(requester));
  }

  ASSERT_NO_THROW(requester->request());
}

TEST(MultiRequester, numTransactions)
{
  using Responder = ResponderPort<int(int)>;

  constexpr size_t num_responders = 10;
  auto requester = std::make_shared<MultiRequesterPort<int(int)>>();

  std::vector<Responder::Ptr> responders;

  for (size_t i = 0; i < num_responders; ++i)
  {
    responders.push_back(std::make_shared<Responder>(
      [](const int v)
      { return 2 * v; })
    );
  }

  for (auto& responder : responders)
  {
    ASSERT_NO_THROW(responder->connect(requester));
    ASSERT_EQ(0, responder->getStatus().num_transactions);
  }

  ASSERT_EQ(0, requester->getStatus().num_transactions);

  constexpr int query = 23;
  const auto responses = requester->request(query);
  ASSERT_EQ(1, requester->getStatus().num_transactions);

  for (auto& responder : responders)
  {
    ASSERT_EQ(1, responder->getStatus().num_transactions);
  }
}