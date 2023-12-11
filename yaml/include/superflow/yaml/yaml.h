// Copyright 2019, Forsvarets forskningsinstitutt. All rights reserved.
#pragma once

#include "superflow/yaml/factory.h"

#include "yaml-cpp/yaml.h"

#include <string>
#include <vector>

namespace flow { class Graph; }

namespace flow::yaml
{
/// Identifies a section in the YAML file.
/// This is a "path" because it can be nested in the Yaml file, e.g.
/// {"Toplevel", "Sublevel", "SubSubLevel", "Proxels"}
using SectionPath = std::vector<std::string>;

/// Sections in the YAML file to look for proxels.
const std::vector<SectionPath> default_proxel_section_paths = {{"Proxels"}};

/// \brief Create a flow::Graph from the given YAML config file.
///
/// The structure and contents of the configuration file must follow a distinct pattern.
/// Two lists are required: \b Proxels and \b Connections.
/// In the `Proxels` list, an entry consists of a unique name, followed by configuration parameters for that proxel.
/// The only required parameter is `type`, which is used as a key to the FactoryMap, i.e. to retrieve the correct Factory.
/// The `Connections` list defines how specific ports on specific proxels are connected together.
///
/// An additional feature included in version 1.5 is the option to include other config files
/// using the \b Includes list. All included files must obey the pattern of a config file, but proxels and connection
/// specifications can be scattered across the union of the files.
/// Only the first file (the one given as argument to this function) is parsed for includes.
///
/// The listing below is a valid example of a configuration file for the yaml module.
/// For further documentation, see the superflow report 19/00776.
/// \code{.yml}
/// %YAML 1.2
/// ---
/// Includes:
///  - sunny_day.yaml
///  - rainy_day.yaml
///
/// Proxels:
///   proxel_1:                # unique name of the proxel
///     type     : "MyProxel"  # class name
///     my_param : 42          # proxel-specific parameter
///
///   proxel_2:                # unique name of the proxel
///     type   : "YourProxel"  # class name
///     enable : false         # leave this proxel out of the graph
///
/// Connections:
///   - [proxel_1: 'out', proxel_2: 'in']
/// ...
/// \endcode
/// \param config_file_path Absolute path to the YAML configuration file
/// \param factory_map Container for mapping Proxel types to their respective Factories
/// \param proxel_section_paths Where to find proxels in the config file. Default is a top level "Proxels" section.
/// \param config_search_directory If the config file utilizes the 'Includes' feature,
///        search for included files relative to this directory.
/// \return A new graph, ready to be started
/// \throws std::runtime_error If the file(s) cannot be found or if something is wrong with the syntax
flow::Graph createGraph(
    const std::string& config_file_path,
    const FactoryMap& factory_map,
    const std::vector<SectionPath>& proxel_section_paths = default_proxel_section_paths,
    const std::string& config_search_directory = {}
);

flow::Graph createGraph(
    const YAML::Node& root,
    const FactoryMap& factory_map,
    const std::vector<SectionPath>& proxel_section_paths = default_proxel_section_paths,
    const std::string& config_search_directory = {}
);

/// \brief Returns a list with the ID of all proxel configs with `flag` set to true.
/// Useful for creating subsets of proxels that require special treatment. For example:
/// \code{.yml}
/// Proxels:
///  my_proxel_1:
///    type: MyProxel
///    my-flag: true
///
///  my_proxel_2:
///    type: MyProxel
///    my-flag: false
/// \endcode
/// `getFlaggedProxels(..., "my-flag")` will then return a list containing only "my_proxel_1".
/// \param root The YAML::Node to parse for flagged proxels
/// \param flag The key to look for, e.g. "my-flag"
/// \param proxel_section_paths Sections in the YAML::Node to look for proxels.
/// \return List of proxels with the `flag` enabled
[[nodiscard]] std::vector<std::string> getFlaggedProxels(
  const YAML::Node& root,
  const std::string& flag,
  const std::vector<SectionPath>& proxel_section_paths = default_proxel_section_paths
);

std::vector<std::string> extractLibPaths(
  const std::string& config_path,
  const std::string& section_name = "LibraryPaths"
);

std::string generateDOTFile(
  const std::string& config_file_path,
  const std::vector<SectionPath>& proxel_section_paths = default_proxel_section_paths,
  const std::string& config_search_directory = {}
);
}
