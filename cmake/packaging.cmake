macro(set_backslash _dst_var _string)
  string(REPLACE "/" "\\" ${_dst_var} ${_string})
endmacro()

macro(unset_provides_conflicts_replaces _component)
  string(TOUPPER ${_component} _COMPONENT)
  set(CPACK_DEBIAN_${_COMPONENT}_PACKAGE_PROVIDES "")
  set(CPACK_DEBIAN_${_COMPONENT}_PACKAGE_CONFLICTS "")
  set(CPACK_DEBIAN_${_COMPONENT}_PACKAGE_REPLACES "")
endmacro()

list(APPEND CMAKE_MODULE_PATH ${CMAKE_SOURCE_DIR}/cmake)
list(REMOVE_DUPLICATES CMAKE_MODULE_PATH)

set(CPACK_PACKAGE_CONTACT "Ragnar Smestad <ragnar.smestad@ffi.no>")
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "An efficient processing framework for modern C++")
set(CPACK_PACKAGE_DESCRIPTION
"The Superflow is the informational space between universes.
 It also serves as the place where dreams and ideas come from, and from where telepathy operates. (...).
 The laws of physics do not apply in the Superflow.")
set(CPACK_PACKAGE_INSTALL_DIRECTORY "superflow")
set(CPACK_PACKAGE_VENDOR "FFI (Forsvarets forskningsinstitutt)")
set(CPACK_RESOURCE_FILE_LICENSE "${CMAKE_SOURCE_DIR}/LICENSE")
set(CPACK_VERBATIM_VARIABLES TRUE)

set(CPACK_DEBIAN_PACKAGE_PROVIDES  "superflow-core,superflow-curses,superflow-loader,superflow-yaml")
set(CPACK_DEBIAN_PACKAGE_CONFLICTS "superflow-core,superflow-curses,superflow-loader,superflow-yaml")
set(CPACK_DEBIAN_PACKAGE_REPLACES  "superflow-core,superflow-curses,superflow-loader,superflow-yaml")
unset_provides_conflicts_replaces("CORE")
unset_provides_conflicts_replaces("CURSES")
unset_provides_conflicts_replaces("LOADER")
unset_provides_conflicts_replaces("YAML")

set(CPACK_DEBIAN_CORE_PACKAGE_DEPENDS "build-essential")
set(CPACK_DEBIAN_CURSES_PACKAGE_DEPENDS "libncurses-dev")
set(CPACK_DEBIAN_LOADER_PACKAGE_DEPENDS "libboost-filesystem-dev")
set(CPACK_DEBIAN_YAML_PACKAGE_DEPENDS "libyaml-cpp-dev")

set(CPACK_DEB_COMPONENT_INSTALL ON)
set(CPACK_DEBIAN_ENABLE_COMPONENT_DEPENDS ON)

set(CPACK_DEBIAN_FILE_NAME DEB-DEFAULT)
set(CPACK_DEBIAN_PACKAGE_DEPENDS "build-essential,libboost-filesystem-dev,libncurses-dev,libyaml-cpp-dev")
set(CPACK_DEBIAN_PACKAGE_HOMEPAGE "https://github.com/ffi-no/superflow")
set(CPACK_DEBIAN_PACKAGE_PRIORITY "optional")
set(CPACK_DEBIAN_PACKAGE_VERSION "${${PROJECT_NAME}_VERSION}")
set(CPACK_DEBIAN_PACKAGE_RELEASE "0")
set(CPACK_DEBIAN_PACKAGE_SECTION "libs")
set(CPACK_DEBIAN_PACKAGE_DESCRIPTION
"${CPACK_PACKAGE_DESCRIPTION_SUMMARY}.
 ${CPACK_PACKAGE_DESCRIPTION}"
)

set(CPACK_NSIS_CONTACT "Ragnar Smestad <ragnar.smestad@ffi.no>")
set(CPACK_NSIS_ENABLE_UNINSTALL_BEFORE_INSTALL ON)
set(CPACK_NSIS_INSTALL_ROOT "C:\\local")
set(CPACK_NSIS_PACKAGE_NAME "superflow")
set(CPACK_NSIS_MUI_ICON    "${CMAKE_SOURCE_DIR}/cmake/nsis-icon.ico")
set(CPACK_NSIS_MUI_UNIICON "${CMAKE_SOURCE_DIR}/cmake/nsis-icon.ico")
set_backslash(CPACK_NSIS_MUI_HEADERIMAGE_BITMAP       "${CMAKE_SOURCE_DIR}/cmake/nsis-banner.bmp")
set_backslash(CPACK_NSIS_MUI_WELCOMEFINISHPAGE_BITMAP "${CMAKE_SOURCE_DIR}/cmake/nsis-welcome.bmp")

if(CMAKE_SYSTEM_NAME STREQUAL "Windows")
  set(CPACK_GENERATOR "NSIS64")
else()
  set(CPACK_GENERATOR "DEB")
endif()

#set(CPACK_COMPONENTS_GROUPING "ONE_PER_GROUP")
#set(CPACK_COMPONENTS_GROUPING "ALL_COMPONENTS_IN_ONE")
#set(CPACK_COMPONENTS_GROUPING "IGNORE")

include(CPack)

cpack_add_component(core   DISPLAY_NAME core   DESCRIPTION "The flow::core library"   GROUP dev)
cpack_add_component(curses DISPLAY_NAME curses DESCRIPTION "The flow::curses library" GROUP dev DEPENDS core)
cpack_add_component(loader DISPLAY_NAME loader DESCRIPTION "The flow::loader library" GROUP dev DEPENDS core)
cpack_add_component(yaml   DISPLAY_NAME yaml   DESCRIPTION "The flow::yaml library"   GROUP dev DEPENDS core)
