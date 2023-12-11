#include "superflow/utils/graphviz.h"

#include <algorithm>
#include <numeric>
#include <sstream>
#include <vector>

namespace flow
{
namespace
{
std::string portFormatting(const std::string& port_name)
{
  return "<" + port_name + "> " + port_name;
}

std::string strToUpper(std::string str)
{
  std::transform(
    str.begin(), str.end(), str.begin(),
    [](unsigned char c)
    { return std::toupper(c); }
  );
  return str;
}

std::string join(const std::set<std::string>& data)
{
  if (data.empty())
  { return {}; }

  return std::accumulate(
    std::next(data.begin()), data.end(),
    portFormatting(*(data.begin())),
    [](std::string a, const std::string& b)
    { return std::move(a) + "| " + portFormatting(b); }
  );
}
}

GraphViz::GraphViz(
  const std::vector<ConnectionSpec>& connections
)
{
  for (const auto& connection : connections)
  {
    insert(connection);
  }
}

void GraphViz::insert(const ConnectionSpec& connection)
{
  if (connection.lhs_name.empty())
  { throw std::invalid_argument("ConnectionSpec must at least have lhs_name."); }

  if (connection.lhs_port.empty() || connection.rhs_name.empty() || connection.rhs_port.empty())
  {
    node_list[connection.lhs_name];
    return;
  }

  node_list[connection.lhs_name].adjacency_list.insert(connection);
  node_list[connection.lhs_name].lhs_ports.insert(connection.lhs_port);
  node_list[connection.rhs_name].rhs_ports.insert(connection.rhs_port);
}

std::string GraphViz::getNodeDefinitions() const
{
  std::ostringstream os;

  for (const auto& [node_name, node]: node_list)
  {
    std::string out = join(node.lhs_ports);
    std::string in = join(node.rhs_ports);
    os << "  " << node_name
       << " [label=\"{"
       << "{ " << in << "} | "
       << strToUpper(node_name) << " | "
       << "{ " << out << "} "
       << "}\"]\n";
  }

  return os.str();
}

std::string GraphViz::getNodeConnections() const
{
  std::ostringstream os;

  for (const auto& [node_name, node]: node_list)
  {
    for (const auto& conn: node.adjacency_list)
    {
      if (conn.lhs_port.empty() || conn.rhs_name.empty() || conn.rhs_port.empty())
      { continue; }

      os << "  " << conn.lhs_name << ":" << conn.lhs_port << " -> " << conn.rhs_name << ":" << conn.rhs_port << "\n";
    }
  }
  return os.str();
}

std::string GraphViz::employ() const
{
  std::ostringstream gv;
  gv << "digraph superflow {\n";
  gv << "  rankdir=\"LR\";\n";
  gv << "  node [shape=Mrecord];\n";
  gv << getNodeDefinitions();
  gv << "\n";
  gv << getNodeConnections();
  gv << "}\n";

  return gv.str();
}
}
