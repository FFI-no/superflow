// Copyright 2019, Forsvarets forskningsinstitutt. All rights reserved.
#include "superflow/yaml/yaml.h"
#include "superflow/yaml/yaml_string_pair.h"

#include "superflow/connection_spec.h"
#include "superflow/graph.h"
#include "superflow/graph_factory.h"
#include "superflow/utils/graphviz.h"
#include "superflow/value.h"

#include <filesystem>
#include <map>
#include <vector>

namespace fs = std::filesystem;
namespace
{
using namespace flow::yaml;

using ReplicaMap = std::map<std::string, size_t>;
using PortSpecification = std::pair<std::string, std::vector<std::string>>;
using ExpandedPortSpecification = std::vector<std::pair<std::string, std::string>>;

ExpandedPortSpecification expandConnectionSpecifier(const PortSpecification& port_spec, size_t proxel_replicas);
std::vector<ProxelConfig> getAllProxelConfigs(const std::vector<YAML::Node>& config_sections);
std::vector<ProxelConfig> getProxelConfigs(const YAML::Node& section);
ProxelConfig createProxelConfig(const std::string& unique_id, const YAML::Node& properties);
std::vector<std::string> getAllProxelNamesFilteredByEnableValue(const std::vector<YAML::Node>& config_sections, bool enable_value);
std::vector<std::string> getProxelNamesFilteredByEnableValue(const YAML::Node& section, bool enable_value);
std::vector<ProxelConfig> getReplicatedConfigs(const std::string& unique_id, size_t num_replicas, const YAML::Node& properties);
ReplicaMap getAllReplicatedProxels(const std::vector<YAML::Node>& config_sections);
std::vector<flow::ConnectionSpec> getConnections(
    const YAML::Node& config,
    const std::vector<std::string>& enabled_proxels,
    const ReplicaMap& replica_map);

bool validConnectionSpecification(const YAML::Node& node);
bool validPortSpecification(const YAML::Node& node);

ReplicaMap getReplicatedProxels(const YAML::Node& section);
size_t getNumberOfProxelReplicas(const YAML::Node& proxel_config);
std::string getProxelReplicaId(const std::string& name, size_t idx);

std::vector<YAML::Node> getProxelSections(
    const YAML::Node& root,
    const std::vector<SectionPath>& proxel_section_paths);
std::vector<YAML::Node> getAllProxelSections(
  const std::vector<YAML::Node>&,
  const std::vector<SectionPath>& proxel_section_paths
);


std::vector<YAML::Node> openAllConfigFiles(
  const std::string& config_file_path,
  const std::string& config_search_directory = {}
);

std::vector<flow::ConnectionSpec> getAllConnections(
  const std::vector<YAML::Node>& config_roots,
  const std::vector<SectionPath>& proxel_section_paths
);

std::vector<std::string> getUnconnectedProxels(
  const std::vector<std::string>& enabled_proxels,
  const std::vector<flow::ConnectionSpec>& all_connections
);

template<typename T>
std::vector<T> operator+(const std::vector<T>& lhs, const std::vector<T>& rhs);

template<typename T>
void operator+=(std::vector<T>& lhs, const std::vector<T>& rhs);

template<typename K, typename V>
std::map<K, V> operator+(const std::map<K, V>& lhs, const std::map<K, V>& rhs);

std::ostream& operator<<(std::ostream& stream, const SectionPath& path);
}

namespace flow::yaml
{
flow::Graph createGraph(
    const std::string& config_file_path,
    const FactoryMap& factory_map,
    const std::vector<SectionPath>& proxel_section_paths,
    const std::string& config_search_directory
)
{
  const auto root = YAML::LoadFile(config_file_path);
  if (config_search_directory.empty())
  {
    const auto parent_path = fs::path{config_file_path}.parent_path().string();
    return createGraph(root, factory_map, proxel_section_paths, parent_path);
  }

  return createGraph(root, factory_map, proxel_section_paths, config_search_directory);
}

flow::Graph createGraph(
    const YAML::Node& root,
    const FactoryMap& factory_map,
    const std::vector<SectionPath>& proxel_section_paths,
    const std::string& config_search_directory
)
{
  YAML::Node node = YAML::Clone(root);
  auto all_config_sections = getProxelSections(node, proxel_section_paths);

  if (node["Includes"])
  {
    for (const auto& included_file : node["Includes"])
    {
      const auto filename = included_file.as<std::string>();
      YAML::Node incl;
      try
      { incl = YAML::LoadFile(filename); }
      catch (const YAML::BadFile&)
      {
        const auto full_path = fs::path{config_search_directory} /= filename;
        incl = YAML::LoadFile(full_path.string());
      }

      if (!incl["Connections"])
      { throw std::invalid_argument("No section 'Connections' specified in file: " + filename); }

      for (const auto& connection : incl["Connections"])
      { node["Connections"].push_back(connection); }

      all_config_sections = all_config_sections + getProxelSections(incl, proxel_section_paths);
    }
  }

  const auto proxel_configurations = getAllProxelConfigs(all_config_sections);
  const auto enabled_proxels = getAllProxelNamesFilteredByEnableValue(all_config_sections, true);
  const auto replicated_proxels = getAllReplicatedProxels(all_config_sections);
  const auto connections = getConnections(node, enabled_proxels, replicated_proxels);

  return flow::createGraph(factory_map, proxel_configurations, connections);
}

std::vector<std::string> getFlaggedProxels(
    const YAML::Node& root,
    const std::string& flag,
    const std::vector<SectionPath>& proxel_section_paths
)
{
  const auto config_sections = getProxelSections(root, proxel_section_paths);
  const auto proxel_configurations = getAllProxelConfigs(config_sections);

  std::vector<std::string> flagged_ids;

  for (const auto& config : proxel_configurations)
  {
    if (!config.properties.hasKey(flag))
    {
      continue;
    }

    if (value<bool>(config.properties, flag))
    {
      flagged_ids.push_back(config.id);
    }
  }

  return flagged_ids;
}

std::vector<std::string> extractLibPaths(
    const std::string& config_path,
    const std::string& section_name
)
{
  const auto config = YAML::LoadFile(config_path);

  if (!config[section_name])
  { throw std::invalid_argument("No " + section_name + " section defined in config."); }

  std::vector<std::string> lib_paths;
  for (const auto& lib_path : config[section_name])
  { lib_paths.push_back(lib_path.as<std::string>()); }

  return lib_paths;
}

std::string generateDOTFile(
  const std::string& config_file_path,
  const std::vector<SectionPath>& proxel_section_paths,
  const std::string& config_search_directory
)
{
  const auto config_roots = openAllConfigFiles(
    config_file_path,
    config_search_directory
  );
  const auto all_connections = getAllConnections(
    config_roots,
    proxel_section_paths
  );

  const auto all_proxel_sections = getAllProxelSections(config_roots, proxel_section_paths);
  const auto enabled_proxels = getAllProxelNamesFilteredByEnableValue(all_proxel_sections, true);
  const auto unconnected_proxels = getUnconnectedProxels(enabled_proxels, all_connections);

  std::vector<ConnectionSpec> dummy_connections;
  for (const auto& proxel_name : unconnected_proxels)
  {
    dummy_connections.push_back({proxel_name,{},{},{}});
  }

  return flow::GraphViz{
    all_connections + dummy_connections
  }.employ();
}
}

// ----- Helper functions -----
namespace
{
std::vector<ProxelConfig> getAllProxelConfigs(const std::vector<YAML::Node>& config_sections)
{
  std::vector<ProxelConfig> configs;

  for (const YAML::Node& section : config_sections)
  {
    configs = configs + getProxelConfigs(section);
  }

  return configs;
}

std::vector<std::string> getAllProxelNamesFilteredByEnableValue(const std::vector<YAML::Node>& config_sections, bool enable_value)
{
  std::vector<std::string> proxel_names;

  for (const YAML::Node& section : config_sections)
  {
    proxel_names = proxel_names + getProxelNamesFilteredByEnableValue(section, enable_value);
  }

  return proxel_names;
}

ReplicaMap getAllReplicatedProxels(const std::vector<YAML::Node>& config_sections)
{
  ReplicaMap replicated_proxels;

  for (const YAML::Node& section : config_sections)
  {
    replicated_proxels = replicated_proxels + getReplicatedProxels(section);
  }

  return replicated_proxels;
}

ProxelConfig createProxelConfig(const std::string& unique_id, const YAML::Node& properties)
{
  const auto type = properties["type"].as<std::string>();
  return {
      unique_id,
      type,
      YAMLPropertyList{properties}
  };
}

std::vector<ProxelConfig> getProxelConfigs(const YAML::Node& section)
{
  std::vector<ProxelConfig> configs;

  for (const auto& p : section)
  {
    const auto enable = (p.second["enable"]) ? p.second["enable"].as<bool>() : true;

    if (!enable)
    { continue; }

    const auto unique_id = p.first.as<std::string>();
    const YAML::Node& properties = p.second;

    const size_t num_replicas = getNumberOfProxelReplicas(properties);

    if (num_replicas > 1)
    {
      configs = configs + getReplicatedConfigs(unique_id, num_replicas, properties);
    }
    else
    {
      configs.push_back(createProxelConfig(unique_id, properties));
    }
  }

  return configs;
}

std::vector<ProxelConfig> getReplicatedConfigs(const std::string& unique_id, const size_t num_replicas, const YAML::Node& properties)
{
  std::vector<ProxelConfig> configs;

  for (size_t idx = 0; idx < num_replicas; ++idx)
  {
    YAML::Node replica_props = YAML::Clone(properties);
    replica_props.remove("replicate");

    for(const auto& kv : properties)
    {
      const auto key = kv.first.as<std::string>();
      const auto value = kv.second;

      if (key.front() == '$')
      {
        if (value.size() != num_replicas)
        {
          throw std::invalid_argument(
              {key + ": properties with '$' requires a list of properties with size equal to number of replicas."}
          );
        }
        replica_props.remove(key);
        replica_props[key.substr(1)] = value[idx];
      }
    }

    const std::string proxel_id = getProxelReplicaId(unique_id, idx);
    configs.push_back(createProxelConfig(proxel_id, replica_props));
  }

  return configs;
}

std::vector<flow::ConnectionSpec> getConnections(
    const YAML::Node& config,
    const std::vector<std::string>& enabled_proxels,
    const ReplicaMap& replica_map)
{
  if (!config["Connections"])
  { throw std::invalid_argument("No section 'Connections' specified in config."); }

  const auto is_enabled = [&enabled_proxels](const std::string& name)
  {
    const auto it = std::find(enabled_proxels.begin(), enabled_proxels.end(), name);
    return it != enabled_proxels.end();
  };

  std::vector<flow::ConnectionSpec> connections;
  connections.reserve(config["Connections"].size());

  for (const auto& connection_pair : config["Connections"])
  {
    if (!validConnectionSpecification(connection_pair))
    {
      throw std::invalid_argument("Bad connection format. Connection must be a YAML::Sequence of two YAML::Maps.\n"
                                  " E.g.: [proxel1: port, proxel2: port]\n"
                                  " or  : [proxel1: [port1, port2], replicated_proxel: port]"
      );
    }

    const auto lhs = connection_pair[0].as<PortSpecification>();
    const auto rhs = connection_pair[1].as<PortSpecification>();

    const std::string& lhs_base_id = lhs.first;
    const std::string& rhs_base_id = rhs.first;

    if (!is_enabled(lhs_base_id) || !is_enabled(rhs_base_id))
    { continue; }

    const auto lhs_replicas = (replica_map.count(lhs_base_id) > 0) ? replica_map.at(lhs_base_id) : 1;
    const auto rhs_replicas = (replica_map.count(rhs_base_id) > 0) ? replica_map.at(rhs_base_id) : 1;

    auto lhs_list = expandConnectionSpecifier(lhs, lhs_replicas);
    auto rhs_list = expandConnectionSpecifier(rhs, rhs_replicas);

    {
      const auto lhs_size = lhs_list.size();
      const auto rhs_size = rhs_list.size();

      if (lhs_size != rhs_size && lhs_size != 1 && rhs_size != 1)
      {
        std::ostringstream ss;
        ss << "Attempted connecting " << lhs_size
           << " ports on " << lhs_base_id
           << " to " << rhs_size << " ports on "
           << rhs_base_id;

        throw std::invalid_argument(ss.str());
      }

      const auto size = std::max(lhs_size, rhs_size);

      if (lhs_size != size)
      {
        lhs_list.resize(size, lhs_list.front());
      }

      if (rhs_size != size)
      {
        rhs_list.resize(size, rhs_list.front());
      }
    }

    for (size_t i = 0; i < lhs_list.size(); ++i)
    {
      connections.push_back(
          {
              lhs_list[i].first, lhs_list[i].second,
              rhs_list[i].first, rhs_list[i].second
          });
    }
  }

  return connections;
}

ExpandedPortSpecification expandConnectionSpecifier(const PortSpecification& port_spec, const size_t proxel_replicas)
{
  const auto& proxel_id = port_spec.first;
  const auto& port_list = port_spec.second;

  if ((proxel_replicas > 1 && port_list.size() > 1))
  {
    throw std::invalid_argument(
        "Ambigous port specification using port list [P: [a,b]] for replicated proxel.  Use e.g."
         "\n - [P: a, P2: ...] "
         "\n - [P: b, P2: ...]"
    );
  }

  ExpandedPortSpecification result;

  if (proxel_replicas > 1)
  {
    for (size_t idx = 0; idx < proxel_replicas; ++idx)
    {
      const auto replica_id = getProxelReplicaId(proxel_id, idx);
      result.emplace_back(replica_id, port_list[0]);
    }
  }
  else if (port_list.size() > 1)
  {
    for (const auto& port : port_list)
    { result.emplace_back(proxel_id, port); }
  }
  else
  {
    result.emplace_back(proxel_id, port_list[0]);
  }

  return result;
}

std::vector<std::string> getProxelNamesFilteredByEnableValue(const YAML::Node& section, const bool enable_value)
{
  std::vector<std::string> proxel_names;

  for (const auto& p : section)
  {
    const auto is_enabled = (p.second["enable"]) ? p.second["enable"].as<bool>() : true;

    if (is_enabled == enable_value)
    {
      const size_t num_replicas = getNumberOfProxelReplicas(p.second);
      const auto unique_id = p.first.as<std::string>();

      if (num_replicas > 1)
      {
        for (size_t idx = 0; idx < num_replicas; ++idx)
        {
          const std::string proxel_id = getProxelReplicaId(unique_id, idx);
          proxel_names.emplace_back(proxel_id);
        }
      }

      proxel_names.emplace_back(unique_id);
    }
  }

  return proxel_names;
}

size_t getNumberOfProxelReplicas(const YAML::Node& proxel_config)
{
  return proxel_config["replicate"]
         ? proxel_config["replicate"].as<size_t>()
         : 1;
}

std::string getProxelReplicaId(
    const std::string& name,
    const size_t idx)
{
  return name + "_" + std::to_string(idx);
}

ReplicaMap getReplicatedProxels(const YAML::Node& section)
{
  ReplicaMap names;

  for (const auto& p : section)
  {
    const size_t num_replicas = getNumberOfProxelReplicas(p.second);

    if (num_replicas > 1)
    {
      const auto unique_id = p.first.as<std::string>();

      names[unique_id] = num_replicas;
    }
  }

  return names;
}

template<typename T>
std::vector<T> operator+(const std::vector<T>& lhs, const std::vector<T>& rhs)
{
  std::vector<T> sum{lhs};
  sum.reserve(lhs.size() + rhs.size());

  sum.insert(sum.end(), rhs.begin(), rhs.end());

  return sum;
}

template<typename T>
void operator+=(std::vector<T>& lhs, const std::vector<T>& rhs)
{
  lhs.insert(lhs.end(), rhs.begin(), rhs.end());
}

template<typename K, typename V>
std::map<K, V> operator+(const std::map<K, V>& lhs, const std::map<K, V>& rhs)
{
  std::map<K, V> sum{lhs};
  sum.insert(rhs.begin(), rhs.end());
  return sum;
}

std::ostream& operator<<(std::ostream& stream, const SectionPath& path)
{
  if (!path.empty())
  {
    stream << path.front();
  }

  for (size_t i = 1; i < path.size(); ++i)
  {
    stream << "/" << path[i];
  }

  return stream;
}

std::vector<YAML::Node> getProxelSections(
    const YAML::Node& root,
    const std::vector<SectionPath>& proxel_section_paths)
{
  std::vector<YAML::Node> sections;
  sections.reserve(proxel_section_paths.size());

  for (const SectionPath& proxel_section_path : proxel_section_paths)
  {
    YAML::Node node = root;

    for (const std::string& step_name : proxel_section_path)
    {
      node.reset(node[step_name]);

      if (!node)
      {
        std::ostringstream ss;
        ss << "no proxel section '" << proxel_section_path << "' defined in config";

        throw std::invalid_argument(ss.str());
      }
    }

    sections.push_back(node);
  }

  return sections;
}

std::vector<YAML::Node>
getAllProxelSections(
  const std::vector<YAML::Node>& config_roots,
  const std::vector<SectionPath>& proxel_section_paths
)
{
  std::vector<YAML::Node> all_proxel_sections;
  for (const auto& node : config_roots)
  {
    all_proxel_sections += getProxelSections(node, proxel_section_paths);
  }
  return all_proxel_sections;
}

std::vector<YAML::Node> openAllConfigFiles(
  const std::string& config_file_path,
  const std::string& config_search_directory
)
{
  std::vector<YAML::Node> config_files;
  const auto include_directory = !config_search_directory.empty()
                                 ? fs::path{config_search_directory}
                                 : fs::path{config_file_path}.parent_path();

  config_files.push_back(YAML::LoadFile(config_file_path));
  const auto& root = config_files.front();

  if (!root["Connections"])
  { throw std::invalid_argument("No section 'Connections' specified in file: " + config_file_path); }

  if (root["Includes"])
  {
    for (const auto& include_entry : root["Includes"])
    {
      const auto filename = include_entry.as<std::string>();
      const auto full_path = fs::is_regular_file(filename) ? filename : (include_directory / filename).string();
      config_files.push_back(YAML::LoadFile(full_path));

      const auto& node = config_files.back();
      if (!node["Connections"])
      { throw std::invalid_argument("No section 'Connections' specified in file: " + full_path); }
    }
  }
  return config_files;
}

std::vector<flow::ConnectionSpec> getAllConnections(
  const std::vector<YAML::Node>& config_roots,
  const std::vector<SectionPath>& proxel_section_paths
)
{
  YAML::Node connections;

  for (const auto& node : config_roots)
  {
    for (const auto& connection : node["Connections"])
    { connections["Connections"].push_back(connection); }
  }

  const auto all_proxel_sections = getAllProxelSections(config_roots, proxel_section_paths);
  const auto enabled_proxels = getAllProxelNamesFilteredByEnableValue(all_proxel_sections, true);
  const auto replicated_proxels = getAllReplicatedProxels(all_proxel_sections);
  auto connection_specs = getConnections(connections, enabled_proxels, replicated_proxels);

  return connection_specs;
}

std::vector<std::string> getUnconnectedProxels(
  const std::vector<std::string>& enabled_proxels,
  const std::vector<flow::ConnectionSpec>& all_connections
)
{
  std::set<std::string> connected_proxels;
  for (const auto& connection: all_connections)
  {
    connected_proxels.insert(connection.lhs_name);
    connected_proxels.insert(connection.rhs_name);
  }

  std::vector<std::string> unconnected_proxels;

  for (const auto& proxel_name : enabled_proxels)
  {
    const auto it = std::find(connected_proxels.begin(), connected_proxels.end(), proxel_name);
    if (it == connected_proxels.end())
    {
      unconnected_proxels.push_back(proxel_name);
    }
  }

  return unconnected_proxels;
}

bool validConnectionSpecification(const YAML::Node& node)
{
  return node.IsSequence()
         && node.size() == 2
         && validPortSpecification(node[0])
         && validPortSpecification(node[1]);
}

bool validPortSpecification(const YAML::Node& node)
{
  return node.IsMap()
         && node.size() == 1
         && (node.begin()->second.IsScalar() || node.begin()->second.IsSequence());
}
}

