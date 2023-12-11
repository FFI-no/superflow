// Copyright 2019, Forsvarets forskningsinstitutt. All rights reserved.
#include "superflow/graph.h"
#include "superflow/utils/metronome.h"

#include <iostream>
#include <sstream>

namespace flow
{
Graph::Graph(std::map<std::string, Proxel::Ptr> proxels)
    : proxels_{std::move(proxels)}
{}

Graph::~Graph()
{
  stop();
}

void Graph::add(const std::string& proxel_id, Proxel::Ptr&& proxel)
{
  if (proxels_.count(proxel_id) > 0)
  { throw std::invalid_argument(std::string("Proxel '" + proxel_id + "' does already exist")); }
  proxels_.emplace(proxel_id, std::move(proxel));
}

void Graph::start(
  const bool handle_exceptions,
  const CrashLogger& crash_logger
)
{
  if (isRunning())
  { throw std::runtime_error("Cannot start Graph when threads are running"); }

  for (const auto& kv : proxels_)
  {
    const auto& proxel_name = kv.first;
    const auto& proxel = kv.second;

    if (handle_exceptions)
    {
      proxel_threads_.emplace(
          proxel_name,
          [this, proxel_name, proxel, crash_logger]()
          {
            try
            {
              proxel->start();
            }
            catch (const std::exception& e)
            {
              crashes_[proxel_name] = e.what();

              if (crash_logger)
              {
                crash_logger(proxel_name, e.what());
              }
            }
            catch (...)
            {
              crashes_[proxel_name] = "unknown exception";

              if (crash_logger)
              {
                crash_logger(proxel_name, "Unknown exception");
              }
            }
          }
      );
    }
    else
    {
      proxel_threads_.emplace(
          proxel_name,
          [&proxel]()
          {
              proxel->start();
          }
      );
    }
  }
}

void Graph::stop()
{
  if (!isRunning())
  { return; }

  for (auto& proxel : proxels_)
  {
    proxel.second->stop();
  }

  for (const auto& kv : proxels_)
  {
    const auto& proxel_name = kv.first;
    const auto thread_it = proxel_threads_.find(proxel_name);

    if (thread_it == proxel_threads_.end())
    {
      continue;
    }

    auto& proc_thread = thread_it->second;

    Metronome repeater{
      [&proxel_name](const auto& duration)
      {
        const auto seconds = std::chrono::duration_cast<std::chrono::seconds>(duration);

        std::cerr << "Still waiting for " << proxel_name << " to finish after " << seconds.count() << "s of waiting" << std::endl;
      },
      std::chrono::seconds{2}
    };

    if (proc_thread.joinable())
    { proc_thread.join(); }

    repeater.stop();
  }

  proxel_threads_.clear();
}

void Graph::connect(
    const std::string& proxel1,
    const std::string& proxel1_port,
    const std::string& proxel2,
    const std::string& proxel2_port) const
{
  if (proxel1 == proxel2)
  {
    throw std::invalid_argument{"Loop detected trying to connect \"" + proxel1 + "\" to itself."};
  }

  try
  {
    auto& port1 = getProxel(proxel1)->getPort(proxel1_port);
    auto& port2 = getProxel(proxel2)->getPort(proxel2_port);

    if (port1 == nullptr)
    { throw std::invalid_argument(proxel1 + "." + proxel1_port + " is a nullptr."); }

    if (port2 == nullptr)
    { throw std::invalid_argument(proxel2 + "." + proxel2_port + " is a nullptr."); }

    port1->connect(port2);
  }
  catch (const std::invalid_argument& e)
  {
    std::ostringstream ss;
    ss << "Connect "
       << proxel1 << "." << proxel1_port
       << " -> "
       << proxel2 << "." << proxel2_port
       << " failed:\n\t"
       << e.what();

    throw std::invalid_argument(ss.str());
  }
}

std::map<std::string, ProxelStatus> Graph::getProxelStatuses() const
{
  std::map<std::string, ProxelStatus> statuses;

  for (const auto& proxel : proxels_)
  {
    const std::string& name = proxel.first;

    auto it = crashes_.find(name);

    if (it != crashes_.end())
    {
      const std::string& error_message = it->second;

      statuses[name] = ProxelStatus{
          ProxelStatus::State::Crashed,
          error_message,
          {}
      };
    } else
    {
      statuses[name] = proxel.second->getStatus();
    }
  }

  return statuses;
}

bool Graph::isRunning() const
{
  return !proxel_threads_.empty();
}

const Graph::CrashLogger Graph::quietCrashLogger = nullptr;

void Graph::defaultCrashLogger(const std::string& proxel_name, const std::string& what)
{
  std::cerr
    << "Proxel '" << proxel_name
    << "' crashed with exception:\n  \""
    << what << "\""
    << std::endl;
}
}
