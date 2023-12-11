// Copyright 2023, Forsvarets forskningsinstitutt. All rights reserved.
#pragma once

#include "superflow/factory_map.h"
#include "boost/dll/shared_library.hpp"
#include <string>

namespace flow::load
{
/// This class will hold a shared library that contains proxels and proxel factories,
/// which can be dynamically loaded using the flow::load module.
/// \note An object of this class must not go out of scope as long as its proxels are in use.
///  That will cause the shared library to be unloaded, and your application to crash!
class ProxelLibrary
{
public:
  /// Load a proxel library from a path, and the library name.
  /// The prefix and suffix of the library file will be computed based on platform (more portable code)
  /// E.g.: library name: myproxels
  /// Linux result: libmyproxels.so
  /// Windows result: myproxels.dll
  ProxelLibrary(
    const boost::dll::fs::path& path_to_directory,
    const std::string& library_name
  );

  /// Load a proxel library by using the full path to the exact library file.
  ProxelLibrary(
    const boost::dll::fs::path& full_path
  );

  /// \brief Load the factories registered with type PropertyList
  /// \tparam PropertyList Some class compatible with flow::value
  /// \tparam Args any optional arguments to the factories (see unit tests for examples)
  /// \param args any optional arguments to the factories (see unit tests for examples)
  /// \return the factories
  /// \throws std::invalid_argument if there are no string PropertyList::adapter_name,
  ///    or there are no proxel factories registered to that adapter name in the library.
  template<typename PropertyList, typename... Args>
  FactoryMap<PropertyList> loadFactories(
    Args... args
  ) const;

private:
  /// Holds the actual shared library
  boost::dll::shared_library library_;

  /// Exception handling are covered here
  ProxelLibrary(
    const boost::dll::fs::path& path,
    boost::dll::load_mode::type load_mode
  );

  [[nodiscard]] std::vector<std::string> getSectionSymbols(const std::string& adapter_name) const;

  /// All factory-symbols are prefixed with the adapter_name + underscore
  [[nodiscard]] static std::string extractFactoryName(const std::string& symbol, const std::string& adapter_name);
};

// -- ProxelLibrary::loadFactories implementation -- //
template<typename PropertyList, typename... Args>
FactoryMap<PropertyList> ProxelLibrary::loadFactories(Args... args) const
{
  typename std::map<std::string, Factory<PropertyList>> factories;

  const auto symbols = getSectionSymbols(PropertyList::adapter_name);
  for (const auto& symbol: symbols)
  {
    auto factory = library_.get_alias < flow::Proxel::Ptr(PropertyList const&, Args...)>(symbol);
    factories.emplace(
      extractFactoryName(symbol, PropertyList::adapter_name),
      [factory = std::move(factory), args...](PropertyList const& adapter)
      {
        return std::invoke(factory, adapter, args...);
      }
    );
  }
  return FactoryMap<PropertyList>{std::move(factories)};
}
}
