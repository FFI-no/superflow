# This macro is meant to be called once for all modules, since it's a set of spells
# that all modules will have to cast to be installed properly.
# It requires arguments in this order:
# - _name  : The name of the generated library
#
# # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # #
include(GNUInstallDirs)

macro(init_module)
  set(_namespace "superflow")
  set(_module "${PROJECT_NAME}")
  set(target_name "${_namespace}-${_module}")

  message(STATUS "* Init module '${_module}'")
endmacro()

macro(add_library_boilerplate)
  set(gcc_compiler_flags
    -Wall
    -Wcast-align
    -Wcast-qual
    -Werror
    -Wextra
    -Wfloat-conversion
    -Winit-self
    -Winit-self
    -Wlogical-op
    -Wmissing-declarations
    -Wnon-virtual-dtor
    -Wold-style-cast
    -Woverloaded-virtual
    -Wpedantic
    -Wpointer-arith
    -Wshadow
    -Wsuggest-override
    -Wuninitialized
    -Wunknown-pragmas
    -Wunreachable-code
    -Wunused-local-typedefs
    )

  set(msvc_compiler_flags
    /W4
    /WX
    )

  list(APPEND built_components ${target_name})
  set(built_components ${built_components} PARENT_SCOPE)

  message(STATUS "* Add cmake boilerplate for target '${target_name}'")

  file(GLOB_RECURSE HEADER_FILES include/*.h)
  file(GLOB_RECURSE SRC_FILES    src/*.cpp)

  add_library(${target_name}
    ${HEADER_FILES}
    ${SRC_FILES}
    )

  add_library(${_namespace}::${_module} ALIAS ${target_name})

  set(msvc_cxx        "$<COMPILE_LANG_AND_ID:CXX,MSVC>")
  set(gcc_like_cxx    "$<COMPILE_LANG_AND_ID:CXX,GNU>")
  set(config_coverage "$<CONFIG:Coverage>")
  set(add_coverage    "$<AND:${config_coverage},${gcc_like_cxx}>")

  target_compile_options(${target_name} PRIVATE
    "$<${gcc_like_cxx}:$<BUILD_INTERFACE:${gcc_compiler_flags}>>"
    "$<${msvc_cxx}:$<BUILD_INTERFACE:${msvc_compiler_flags}>>"
    )

  target_compile_options(${target_name} PUBLIC "$<${add_coverage}:--coverage;-g;-O0>")
  target_link_options(   ${target_name} PUBLIC "$<${add_coverage}:--coverage>")

  target_include_directories(${target_name}
    PRIVATE $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
  )

  target_include_directories(${target_name}
    SYSTEM INTERFACE
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>
  )

  set_target_properties(${target_name} PROPERTIES
    LINKER_LANGUAGE CXX
    CXX_STANDARD_REQUIRED ON
    CXX_STANDARD 17
    POSITION_INDEPENDENT_CODE ON
    BUILD_RPATH $ORIGIN
    INSTALL_RPATH $ORIGIN
    WINDOWS_EXPORT_ALL_SYMBOLS ON
    )

  # Prepend the top-level project name to all installed library files
  string(TOLOWER "${target_name}" output_name)
  set_target_properties(${target_name} PROPERTIES
    EXPORT_NAME ${_module}
    OUTPUT_NAME ${output_name}
    )

  common_boilerplate()

endmacro()

macro(common_boilerplate)
  set(project_config_in   "${CMAKE_CURRENT_LIST_DIR}/${_module}-config.cmake.in")
  set(project_config_out  "${CMAKE_BINARY_DIR}/${_namespace}-${_module}-config.cmake")

  configure_package_config_file(
    ${project_config_in}
    ${project_config_out}
    INSTALL_DESTINATION ${config_install_dir}
    NO_SET_AND_CHECK_MACRO
    NO_CHECK_REQUIRED_COMPONENTS_MACRO
    )

  install(FILES
    ${project_config_out}
    DESTINATION ${config_install_dir}
    COMPONENT ${_module}
    )

  install(TARGETS ${target_name}
    EXPORT ${targets_export_name}
    COMPONENT ${_module}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    INCLUDES DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
    )

  install(
    DIRECTORY "include/"
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
    COMPONENT ${_module}
    FILES_MATCHING PATTERN "*.h"
    )
endmacro()
