// Copyright 2019, Forsvarets forskningsinstitutt. All rights reserved.
/// \file register-factory.h
/// \brief Macros for creating proxel plugin libraries.
///
/// See #REGISTER_PROXEL_FACTORY(ProxelName, Factory).

/// \def REGISTER_PROXEL_FACTORY(ProxelName, Factory)
/// Register the symbol of the Factory function \a Factory to the alias \a ProxelName.
///
/// The macro \a LOADER_ADAPTER_NAME must be defined and will be used as the
/// shared library section name.
///
/// The macro \a LOADER_ADAPTER_TYPE must be defined and set to a
/// valid implementation of ValueAdapter when compiling the proxel plugin library.
///
/// The macro \a LOADER_ADAPTER_HEADER may be defined and set to a
/// header file containing the definition of LOADER_ADAPTER_TYPE.
///
/// \note The library flow::yaml will define the macros in its cmake INTERFACE_COMPILE_DEFINITIONS,
/// so you should not have to worry about them.
/// Thus, the following example is contrived, but illustrates the mechanism:
///
///       \code{.cpp}
///       #define LOADER_ADAPTER_NAME YAML
///       #define LOADER_ADAPTER_TYPE flow::yaml::YAMLPropertyList
///       #define LOADER_ADAPTER_HEADER "superflow/yaml/yaml_property_list.h"
///       // ...
///       REGISTER_PROXEL_FACTORY(MyProxel, createMyProxel)
///
/// \param ProxelName The class name of the Proxel you want to register a factory for.
/// \param Factory The flow::Factory function that can create a \a ProxelName wrapped in a Proxel::Ptr.

/// \def LOADER_ADAPTER_TYPE
/// The actual type of PropertyList, e.g. flow::yaml::YAMLPropertyList

#pragma once

#include "boost/dll/alias.hpp"
#include "boost/preprocessor/stringize.hpp"

#ifdef LOADER_ADAPTER_HEADER
#include LOADER_ADAPTER_HEADER
#endif

/// Helper macro
#define ALIAS(SECTION, NAME)  \
        BOOST_PP_CAT(BOOST_PP_CAT(SECTION, _), NAME)

/// \brief Register the symbol of the Factory function \a Factory to the alias \a ProxelName
/// within the shared library section \a Section.
/// Helper macros like REGISTER_PROXEL_FACTORY, or macros for specific adapters like
/// REGISTER_YAML_PROXEL_FACTORY (defined elsewhere)
/// will call this macro with a predefined section name.
/// Typically not used directly, but an example would be
///
///    \code{.cpp}
///    REGISTER_PROXEL_FACTORY_SECTIONED(YAML_MyProxel, createMyProxel<flow::yaml::YamlValueAdapter>, YAML)
///
#define REGISTER_PROXEL_FACTORY_SECTIONED(ProxelName, Factory, Section) \
        BOOST_DLL_ALIAS_SECTIONED(                                     \
          Factory,                                                     \
          ALIAS(Section, ProxelName),                                  \
          Section                                                      \
        )

#ifdef LOADER_IGNORE
#define REGISTER_PROXEL_FACTORY(ProxelName, Factory)
#elif defined(LOADER_ADAPTER_NAME) && defined(LOADER_ADAPTER_TYPE)
#define REGISTER_PROXEL_FACTORY(ProxelName, Factory) \
          REGISTER_PROXEL_FACTORY_SECTIONED(ProxelName, Factory<LOADER_ADAPTER_TYPE>, LOADER_ADAPTER_NAME)
#else
#define REGISTER_PROXEL_FACTORY(ProxelName, Factory) \
    _Pragma(BOOST_PP_STRINGIZE(message \
      "LOADER_ADAPTER_NAME and LOADER_ADAPTER_TYPE are not defined. "\
      "REGISTER_PROXEL_FACTORY(" BOOST_PP_STRINGIZE(ProxelName) ", " BOOST_PP_STRINGIZE(Factory) ") has no effect."\
  ))
#endif
