#pragma once

#include "superflow/connection_spec.h"

#include <map>
#include <set>
#include <unordered_set>
#include <vector>

template<>
struct std::hash<flow::ConnectionSpec>
{
  std::size_t operator()(const flow::ConnectionSpec& c) const noexcept
  {
    std::size_t h1 = std::hash<std::string>{}(c.lhs_name);
    std::size_t h2 = std::hash<std::string>{}(c.lhs_port);
    std::size_t h3 = std::hash<std::string>{}(c.rhs_name);
    std::size_t h4 = std::hash<std::string>{}(c.rhs_port);
    return h1 ^ (h2 << 1) ^ (h3 << 2) ^ (h4 << 3);
  }
};

template<>
struct std::equal_to<flow::ConnectionSpec>
{
  bool operator()(
    const flow::ConnectionSpec& lhs,
    const flow::ConnectionSpec& rhs
  ) const
  {
    return
      lhs.lhs_name == rhs.lhs_name &&
      lhs.lhs_port == rhs.lhs_port &&
      lhs.rhs_name == rhs.rhs_name &&
      lhs.rhs_port == rhs.rhs_port;
  }
};

namespace flow
{
/// \brief Parse ConnectionSpec%s to create graphviz source code for rendering with dot
///
/// \code{.cpp}
/// std::ofstream{"./superflow.gv"}  &lt;&lt; flow::yaml::generateDOTFile(config_file_path) &lt;&lt; std::endl;
/// \endcode
///
/// \code{.sh}
/// dot -Tsvg -o graph.svg superflow.gv
/// \endcode
class GraphViz
{
public:
  /// Build the internal data structure parsed from the ConnectionSpec%s.
  ///
  /// We also support dummy ConnectionSpec%s like {proxel_name,{},{},{}},
  /// so that unconnected proxels can also be rendered.
  /// \param connections
  GraphViz(
    const std::vector<ConnectionSpec>& connections
  );

  /// Render the DOT-file source code
  std::string employ() const;

private:
  using AdjacencyList = std::unordered_set<ConnectionSpec>;

  struct ProxelMeta
  {
    AdjacencyList adjacency_list;
    std::set<std::string> lhs_ports;
    std::set<std::string> rhs_ports;
  };

  std::map<std::string, ProxelMeta> node_list;

  void insert(const ConnectionSpec& connection);
  std::string getNodeDefinitions() const;
  std::string getNodeConnections() const;
};
}
