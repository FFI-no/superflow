// Copyright 2019, Forsvarets forskningsinstitutt. All rights reserved.
#include "superflow/utils/signal_waiter.h"

#include <map>
#include <set>

namespace flow
{
namespace
{
using Handler = std::shared_ptr<std::function<void()>>;

std::mutex handler_mutex;
std::map<int, std::set<Handler>> handlers;

void register_handler(int signal, const Handler& handler);

void unregister_handler(int signal, const Handler& handler);

void handle_signal(int signal);
}

SignalWaiter::SignalWaiter(
  const std::vector<int>& signals
)
  : is_waiting_{true}
  , got_signal_{false}
  , handler_{std::make_shared<std::function<void()>>([this](){handleSignal();})}
{
  for (const auto signal : signals)
  {
    register_handler(signal, handler_);
  }

  worker_ = std::async(
    std::launch::async,
    [this, signals]()
    {
      await(signals);
    }
  );
}

SignalWaiter::~SignalWaiter()
{
  is_waiting_ = false;
  cv_.notify_one();
  worker_.wait();
}

bool SignalWaiter::hasGottenSignal() const
{
  return got_signal_;
}

std::shared_future<void> SignalWaiter::getFuture() const
{
  return worker_;
}

void SignalWaiter::handleSignal()
{
  got_signal_ = true;
  cv_.notify_one();
}

void SignalWaiter::await(const std::vector<int>& signals)
{
  std::unique_lock<std::mutex> lock{mutex_};
  cv_.wait(
    lock,
    [this]()
    {
      return !is_waiting_ || got_signal_;
    }
  );

  for (const auto signal : signals)
  {
    unregister_handler(signal, handler_);
  }
}

namespace
{
void register_handler(
  const int signal,
  const Handler& handler
)
{
  std::lock_guard<std::mutex> lock{handler_mutex};

  auto it = handlers.find(signal);

  if (it == handlers.end())
  {
    it = handlers.insert({signal, {}}).first;
    std::signal(signal, handle_signal);
  }

  it->second.insert(handler);
}

void unregister_handler(
  const int signal,
  const Handler& handler
)
{
  std::lock_guard<std::mutex> lock{handler_mutex};

  const auto it = handlers.find(signal);

  if (it == handlers.end())
  {
    return;
  }

  it->second.erase(handler);

  if (it->second.empty())
  {
    // no handlers are registered with this signal
    // so set it back to default

    std::signal(signal, SIG_DFL);
    handlers.erase(it);
  }
}

void handle_signal(const int signal)
{
  std::lock_guard<std::mutex> lock{handler_mutex};

  const auto it = handlers.find(signal);

  if (it == handlers.end())
  {
    return;
  }

  for (const auto& handler : it->second)
  {
    (*handler)();
  }
}
}
}
