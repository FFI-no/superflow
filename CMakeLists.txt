cmake_minimum_required(VERSION 3.10.2)
project(superflow VERSION 4.0.1)
message(STATUS "* Generating '${PROJECT_NAME}' v${${PROJECT_NAME}_VERSION}")

set(CMAKE_DEBUG_POSTFIX "d")
set_property(GLOBAL PROPERTY USE_FOLDERS ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

string(TOLOWER ${CMAKE_PROJECT_NAME} package_name)
set(config_install_dir  "share/cmake/${package_name}/")
set(namespace           "${package_name}::")
set(project_config_in   "${CMAKE_CURRENT_LIST_DIR}/cmake/config.cmake.in")
set(project_config_out  "${CMAKE_BINARY_DIR}/${package_name}-config.cmake")
set(targets_export_name "${package_name}-targets")
set(version_config_out  "${CMAKE_BINARY_DIR}/${package_name}-config-version.cmake")

option(BUILD_TESTS  "Whether or not to build the tests" OFF)
option(BUILD_all    "build superflow with all submodules" ON)
option(BUILD_curses "build submodule curses" OFF)
option(BUILD_loader "build submodule loader" OFF)
option(BUILD_yaml   "build submodule yaml"   OFF)

if (BUILD_TESTS)
  include(cmake/enable-tests.cmake)
endif()

include(${CMAKE_BINARY_DIR}/conan_paths.cmake OPTIONAL)
include(${CMAKE_CURRENT_SOURCE_DIR}/build/conan_paths.cmake OPTIONAL)
list(APPEND CMAKE_MODULE_PATH ${CMAKE_CURRENT_BINARY_DIR})

include(CMakePackageConfigHelpers)
include(cmake/cmake-boilerplate.cmake)

add_subdirectory(core)

if (NOT MSVC AND (BUILD_curses OR BUILD_all))
  add_subdirectory(curses)
endif()
if (BUILD_loader OR BUILD_all)
  add_subdirectory(loader)
endif()
if (BUILD_yaml OR BUILD_all)
  add_subdirectory(yaml)
endif()

# # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # #

configure_package_config_file(
  ${project_config_in}
  ${project_config_out}
  INSTALL_DESTINATION ${config_install_dir}
  PATH_VARS built_components
  NO_SET_AND_CHECK_MACRO
  )

write_basic_package_version_file(
  ${version_config_out}
  COMPATIBILITY SameMajorVersion
  )

install(FILES
  ${project_config_out}
  ${version_config_out}
  DESTINATION ${config_install_dir}
  COMPONENT core
  )

install(FILES
  ${CMAKE_SOURCE_DIR}/LICENSE
  DESTINATION "${CMAKE_INSTALL_DOCDIR}"
  COMPONENT core
  )

install(EXPORT ${targets_export_name}
  NAMESPACE ${namespace}
  DESTINATION ${config_install_dir}
  COMPONENT core
  )

export(
  TARGETS ${built_components}
  NAMESPACE ${namespace}
  FILE ${CMAKE_BINARY_DIR}/${targets_export_name}.cmake
  )

# # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # #
# Create target 'doxygen'
find_package(Doxygen)
if (Doxygen_FOUND)
  add_custom_target(doxygen
    COMMAND
      ${PROJECT_NAME}_VERSION=v${${PROJECT_NAME}_VERSION}
      ${DOXYGEN_EXECUTABLE}
      ${CMAKE_CURRENT_LIST_DIR}/doc/Doxyfile
    WORKING_DIRECTORY ${CMAKE_CURRENT_LIST_DIR}
    COMMENT "Generating API documentation with Doxygen"
    VERBATIM
    )
endif()

# # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # #
# Create target 'pack'
get_cmake_property(CPACK_COMPONENTS_ALL COMPONENTS)
include(cmake/packaging.cmake)

add_custom_target(pack
  COMMAND
    ${CMAKE_CPACK_COMMAND} "-G" "DEB"
    "-D" "CPACK_COMPONENTS_GROUPING=ALL_COMPONENTS_IN_ONE"
  # Denne virker ikke. Alle pakkene blir "installert" likevel.
  #COMMAND
  #  ${CMAKE_CPACK_COMMAND} "-G" "DEB"
  #  "-D" "CPACK_COMPONENTS_GROUPING=IGNORE"
  COMMENT "Running CPack. Please wait..."
  DEPENDS ${CPACK_COMPONENTS_ALL}
  WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
  )

# # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # #
# Create target 'uninstall'
add_custom_target(uninstall
  "${CMAKE_COMMAND}" -P "${CMAKE_CURRENT_LIST_DIR}/cmake/uninstall.cmake"
)
