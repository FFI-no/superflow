// Copyright 2019, Forsvarets forskningsinstitutt. All rights reserved.
#pragma once

#include "superflow/proxel.h"
#include "superflow/port.h"

#include <functional>
#include <map>
#include <thread>
#include <vector>

namespace flow
{

/// \brief Processing graph responsible for starting, stopping and monitoring Proxels
///
/// Graph is the manager of Proxels, mainly providing a data structure for them to exist
/// and in the current implementation providing them with worker threads.
/// Graph can also be queried for Proxel statuses to monitor workload and processing times.
/// \see Proxel
class Graph
{
public:
  /// A function which is called if a Proxel chrashes, and the graph handles exceptions.
  /// \param proxel_name First argument, the name of the crashed proxel
  /// \param what Second argument, the `what` of the caught exception.
  using CrashLogger = std::function<void(const std::string& proxel_name, const std::string& what)>;

  Graph() = default;

  /// Constructor that creates graph with a predefined set of proxels.
  /// Proxels will otherwise have to be added using the `add` method.
  /// \param proxels map which maps unique names to Proxels.
  explicit Graph(std::map<std::string, Proxel::Ptr> proxels);

  Graph(Graph&&) = default;

  Graph(const Graph&) = delete;

  ~Graph();

  /// Call the `start` method of every Proxel, using a new thread per element.
  /// Threads are not detatched.
  /// \param handle_exceptions true if Graph should catch exceptions from proxels
  /// \param crash_logger function that logs error messages caught from proxels. Has effect only
  /// if `handle_exceptions` is also `true`.
  /// \see CrashLogger, defaultCrashLogger
  void start(
    bool handle_exceptions = true,
    const CrashLogger& crash_logger = defaultCrashLogger
  );

  /// Call the `stop` method of every Proxel, expecting the proxel thread to terminate.
  /// Threads are joined.
  void stop();

  /// \brief Add a new proxel to the Graph.
  /// \param proxel_id Unique name for the Proxel
  /// \param proxel pointer to the proxel
  void add(const std::string& proxel_id, Proxel::Ptr&& proxel);

  /// \brief Request a pointer to a Proxel
  /// \tparam ProxelType optional template argument in order to get a specific sub class of Proxel.
  /// \param proxel_name Unique name of Proxel
  /// \return A pointer to the requested Proxel
  /// \throws std::invalid argument if Proxel does not exist or is not of requested type.
  template<typename ProxelType = Proxel>
  std::shared_ptr<ProxelType> getProxel(const std::string& proxel_name) const;

  /// \brief Create a connection between ports in two Proxels.
  /// \param proxel1 Unique name of the first Proxel
  /// \param proxel1_port Unique name of the first Proxel's port
  /// \param proxel2 Unique name of the second Proxel
  /// \param proxel2_port Unique name of the second Proxel's port
  void connect(const std::string& proxel1, const std::string& proxel1_port,
               const std::string& proxel2, const std::string& proxel2_port) const;

  /// \brief Retreive the current status of all proxels.
  /// \see ProxelStatusMap
  /// \return
  [[nodiscard]] ProxelStatusMap getProxelStatuses() const;

  /// A CrashLogger that prints the error message to std::cerr.
  /// \see CrashLogger
  static void defaultCrashLogger(const std::string& proxel_name, const std::string& what);

  /// A CrashLogger that does absolutely nothing
  /// \see CrashLogger
  static const CrashLogger quietCrashLogger;

private:
  std::map<std::string, Proxel::Ptr> proxels_;
  std::map<std::string, std::string> crashes_;

  std::map<std::string, std::thread> proxel_threads_;

  [[nodiscard]] bool isRunning() const;
};

template<typename ProxelType>
std::shared_ptr<ProxelType> Graph::getProxel(const std::string& proxel_name) const
{
  const Proxel::Ptr raw_ptr = getProxel(proxel_name); // does not recurse infinitely
  const auto ptr = std::dynamic_pointer_cast<ProxelType>(raw_ptr);

  if (ptr == nullptr)
  { throw std::invalid_argument({"Proxel '" + proxel_name + "' is not of requested type."}); }

  return ptr;
}

template<>
inline Proxel::Ptr Graph::getProxel(const std::string& proxel_name) const
{
  try
  { return proxels_.at(proxel_name); }
  catch (...)
  { throw std::invalid_argument(std::string("Proxel '" + proxel_name + "' does not exist")); }
}
}
