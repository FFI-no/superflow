#pragma once

#include "superflow/loader/proxel_library.h"

namespace flow::load
{
/// \brief Collect factories across multiple ProxelLibrary%s and concatenate into a single FactoryMap
/// \see ProxelLibrary::loadFactories
template<typename PropertyList, typename... Args>
FactoryMap<PropertyList> loadFactories(
  const std::vector<ProxelLibrary>& libraries,
  Args... args
)
{
  FactoryMap<PropertyList> factories;

  for (const auto& library : libraries)
  {
    factories += library.loadFactories<PropertyList, Args...>(args...);
  }

  return factories;
}

// It is disallowed to construct the vector as a temporary within the function argument list,
// as this would cause the library to be unloaded immediately after return.
// The std::is_same is there to make the static_assert dependent on the template parameter,
// otherwise it would cause a compilation error even when the overload is not called.
template<typename PropertyList, typename... Args>
FactoryMap<PropertyList> loadFactories(
  const std::vector<ProxelLibrary>&&,
  Args...
)
{
  static_assert(
    !std::is_same<PropertyList, PropertyList>::value, // always false
    "No temporary allowed!"
    "You must keep the libraries in scope, so they won't be unloaded while you attempt to use them.");
}
}
