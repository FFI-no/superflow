// Copyright 2019, Forsvarets forskningsinstitutt. All rights reserved.
#pragma once

#include "yaml-cpp/node/convert.h"

#include <string>
#include <vector>

namespace YAML
{
template<>
struct convert<std::pair<std::string, std::string>>
{
  static Node encode(const std::pair<std::string, std::string>& rhs)
  {
    Node node(NodeType::Sequence);
    node.push_back(rhs.first);
    node.push_back(rhs.second);
    return node;
  }

  static bool decode(const Node& node, std::pair<std::string, std::string>& rhs)
  {
    if (node.IsMap() && node.size() == 1)
    {
      rhs.first = node.begin()->first.as<std::string>();
      rhs.second = node.begin()->second.as<std::string>();
      return true;
    }

    return false;
  }
};

template<>
struct convert<std::pair<std::string, std::vector<std::string>>>
{
  static Node encode(const std::pair<std::string, std::vector<std::string>>& pair)
  {
    Node node(NodeType::Sequence);

    node.push_back(pair.first);
    node.push_back(pair.second);

    return node;
  }

  static bool decode(const Node& node, std::pair<std::string, std::vector<std::string>>& rhs)
  {
    if (node.IsMap() && node.size() == 1)
    {
      rhs.first = node.begin()->first.as<std::string>();

      const auto& second = node.begin()->second;
      rhs.second = second.IsSequence()
                   ? second.as<std::vector<std::string>>()
                   : std::vector<std::string>{second.as<std::string>()};

      return true;
    }

    return false;
  }
};
}
