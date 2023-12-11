// Copyright 2019, Forsvarets forskningsinstitutt. All rights reserved.
#include "superflow/utils/signal_waiter.h"

#include "gtest/gtest.h"

#include <chrono>
#include <thread>

using namespace flow;

TEST(SignalWaiter, waits)
{
  SignalWaiter waiter;

  ASSERT_FALSE(waiter.hasGottenSignal());

  std::raise(SIGINT);

  ASSERT_TRUE(waiter.hasGottenSignal());
}

TEST(SignalWaiter, stopsWaitingOnDTOR)
{
  std::shared_future<void> waiter_future;
  {
    SignalWaiter waiter;
    ASSERT_FALSE(waiter.hasGottenSignal());
    waiter_future = waiter.getFuture();
  }
  waiter_future.get();
}

namespace
{
constexpr size_t num_workers = 50;

auto createSignalWaiterWorkers(int signal)
{
  std::vector<std::future<void>> workers;
  std::vector<uint8_t> workers_done(num_workers, false);
  std::vector<std::promise<void>> workers_has_started(num_workers);
  std::vector<std::promise<void>> workers_has_ended(num_workers);

  for (size_t i = 0; i < num_workers; ++i)
  {
    workers.push_back(
      std::async(
        std::launch::async,
        [&worker_done = workers_done[i],
         &worker_has_started = workers_has_started[i],
         &worker_has_ended = workers_has_ended[i],
         signal]()
        {
          SignalWaiter waiter{{signal}};
          worker_has_started.set_value();
          waiter.getFuture().wait();

          worker_done = true;
          worker_has_ended.set_value();
        }
      )
    );
  }

  return std::make_tuple(std::move(workers),
                         std::move(workers_done),
                         std::move(workers_has_started),
                         std::move(workers_has_ended));
}

bool all(const std::vector<uint8_t>& values)
{
  return std::all_of(
    values.begin(),
    values.end(),
    [](uint8_t status)
    { return status; }
  );
}

void waitFor(std::vector<std::promise<void>>& promises_to_wait_for)
{
  for (auto& promise: promises_to_wait_for)
  {
    promise.get_future().wait();
  }
}
}

TEST(SignalWaiter, multiThreaded)
{
  auto [sigint_workers,
    sigint_workers_done,
    sigint_workers_has_started,
    sigint_workers_has_ended] = createSignalWaiterWorkers(SIGINT);

  auto [sigterm_workers,
    sigterm_workers_done,
    sigterm_workers_has_started,
    sigterm_workers_has_ended] = createSignalWaiterWorkers(SIGTERM);

  waitFor(sigint_workers_has_started);
  ASSERT_FALSE(all(sigint_workers_done));
  std::raise(SIGINT);
  waitFor(sigint_workers_has_ended);
  ASSERT_TRUE(all(sigint_workers_done));

  waitFor(sigterm_workers_has_started);
  ASSERT_FALSE(all(sigterm_workers_done));
  std::raise(SIGTERM);
  waitFor(sigterm_workers_has_ended);
  ASSERT_TRUE(all(sigterm_workers_done));
}

