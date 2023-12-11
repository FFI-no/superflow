// Copyright 2019, Forsvarets forskningsinstitutt. All rights reserved.
#pragma once

#include "yaml-cpp/yaml.h"

#include <stdexcept>

namespace flow::yaml
{
class YAMLPropertyList
{
public:
  YAMLPropertyList() = default;

  explicit YAMLPropertyList(const YAML::Node& parent);

  bool hasKey(const std::string& key) const;

  template<typename T>
  T convertValue(const std::string& key) const;

  static constexpr const char* adapter_name{"YAML"};

private:
  YAML::Node parent_;
};

// ----- Implementation -----
inline YAMLPropertyList::YAMLPropertyList(const YAML::Node& parent)
    : parent_{parent}
{
  if (!parent.IsMap())
  {
    throw std::invalid_argument("Input node to YAMLPropertyList must be a YAML::Map.");
  }
}

inline bool YAMLPropertyList::hasKey(const std::string& key) const
{
  return bool(parent_[key]);
}

template<typename T>
T YAMLPropertyList::convertValue(const std::string& key) const
{
  const auto& node = parent_[key];
  if (!node)
  {
    throw std::runtime_error({"Could not find key \"" + key + "\" in property list"});
  }

  try
  {
    return node.as<T>();
  }
  catch (const YAML::TypedBadConversion<T>&)
  {
    throw std::runtime_error({"Type mismatch for key: \"" + key + "\""});
  }
  catch (const YAML::BadConversion&)
  {
    throw std::runtime_error({"Failed to parse key: \"" + key + "\""});
  }
}
}
