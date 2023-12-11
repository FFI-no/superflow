// Copyright 2019, Forsvarets forskningsinstitutt. All rights reserved.
#pragma once

#include <string>

namespace flow
{
/// \brief Extract a value from a PropertyList.
///
/// A PropertyList is a templated type that must support extraction of values by referring to their names.
/// It is required that a valid PropertyList defines the following functions:
/// <ul>
/// <li> `bool hasKey(const std::string& key)`, tells whether the given key exists or not. </li>
/// <li> `T convertValue(const std::string& key)`, retrieves a value from the list. </li>
/// </ul>
/// \tparam T The value type
/// \tparam PropertyList The concrete type of PropertyList
/// \param properties the property list
/// \param key The identifier of the value
/// \return The value.
/// \note May throw if the key does not exist (depends on the implementation of PropertyList)
template<typename T, typename PropertyList>
T value(const PropertyList& properties, const std::string& key)
{
  return properties.template convertValue<T>(key);
}

/// Same as above, but will return default_value if the 'key' does not exist.
template<typename T, typename PropertyList, typename ... Args>
T value(const PropertyList& properties, const std::string& key, const T& default_value)
{
  return properties.hasKey(key)
         ? properties.template convertValue<T>(key)
         : default_value;
}

/// Same as above, but will return a default value constructed by 'default_args' if 'key' does not exist.
template<typename T, typename PropertyList, typename ... Args>
T value(const PropertyList& properties, const std::string& key, Args... default_args)
{
  return properties.hasKey(key)
         ? properties.template convertValue<T>(key)
         : T{default_args...};
}
}
