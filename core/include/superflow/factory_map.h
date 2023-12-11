// Copyright 2019, Forsvarets forskningsinstitutt. All rights reserved.
#pragma once

#include "superflow/factory.h"

#include <map>
#include <string>

namespace flow
{
/// \brief Container for mapping a Proxel type to its respective Factory
/// \tparam PropertyList
template<typename PropertyList>
class FactoryMap
{
public:
  FactoryMap() = default;

  FactoryMap(FactoryMap&&) noexcept = default;

  FactoryMap(const FactoryMap&) = default;

  FactoryMap& operator=(FactoryMap&&) noexcept = default;

  FactoryMap& operator=(const FactoryMap&) = default;

  /// \brief Create a new FactoryMap
  /// \param factories mapping between proxel type and factory.
  explicit FactoryMap(const std::map<std::string, Factory<PropertyList>>& factories);

  explicit FactoryMap(std::map<std::string, Factory<PropertyList>>&& factories);

  /// \brief Get Factory for the given Proxel type.
  /// \param type The name of the Proxel type
  /// \return The Proxel type's Factory
  const Factory<PropertyList>& get(const std::string& type) const;

  /// \brief Concatenate two FactoryMaps
  /// \param other the FactoryMap to combine with the current
  /// \return A new, concatenated FactoryMap
  FactoryMap operator+(const FactoryMap& other) const;
  void operator+=(const FactoryMap& other);

  [[nodiscard]] bool empty() const { return factories_.empty(); }

private:
  std::map<std::string, Factory<PropertyList>> factories_;
};

// ----- Implementation -----

template<typename PropertyList>
FactoryMap<PropertyList>::FactoryMap(const std::map<std::string, Factory<PropertyList>>& factories)
    : factories_{factories}
{}

template<typename PropertyList>
FactoryMap<PropertyList>::FactoryMap(std::map<std::string, Factory<PropertyList>>&& factories)
    : factories_{std::move(factories)}
{}

template<typename PropertyList>
const Factory<PropertyList>& FactoryMap<PropertyList>::get(const std::string& type) const
{
  try
  { return factories_.at(type); }
  catch (std::exception&)
  { throw std::invalid_argument({"FactoryMap failed to load factory '" + type + "'"}); }
}

template<typename PropertyList>
FactoryMap<PropertyList> FactoryMap<PropertyList>::operator+(const FactoryMap<PropertyList>& other) const
{
  std::map<std::string, Factory<PropertyList>> sum{factories_.begin(), factories_.end()};

  sum.insert(other.factories_.begin(), other.factories_.end());

  return FactoryMap{std::move(sum)};
}

template<typename PropertyList>
void FactoryMap<PropertyList>::operator+=(const FactoryMap& other)
{
  factories_.insert(other.factories_.begin(), other.factories_.end());
}
}
