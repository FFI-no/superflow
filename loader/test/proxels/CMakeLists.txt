cmake_minimum_required(VERSION 3.10.2 FATAL_ERROR)

project(proxels CXX)
message(STATUS "*  Adding test library 'proxels'")

if (MSVC)
  set(CMAKE_DEBUG_POSTFIX "")
  foreach(config ${CMAKE_CONFIGURATION_TYPES})
    string(TOUPPER ${config} CONFIG)
    set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_${CONFIG} ${CMAKE_CURRENT_BINARY_DIR})
  endforeach()
endif()

add_library(dummy-adapter STATIC)
add_library(superflow::dummy-adapter ALIAS dummy-adapter)
target_sources(dummy-adapter
  PRIVATE dummy_value_adapter.cpp
  PUBLIC FILE_SET public_headers
  TYPE HEADERS
  BASE_DIRS include
  FILES
  include/testing/dummy_value_adapter.h
)
target_link_libraries(dummy-adapter
  PUBLIC superflow::loader
)


set_target_properties(dummy-adapter PROPERTIES
    CXX_STANDARD_REQUIRED ON
    CXX_STANDARD 17
    )

set(dummy_adapter_name "DUMMY")
target_compile_definitions(dummy-adapter
  PUBLIC
  DUMMY_ADAPTER_NAME=${dummy_adapter_name}
)

option(SET_LOADER_ADAPTER "Set the LOADER_ADAPTER_xxx macros. Turn off if you are linking more than one adapter." ON)
if (SET_LOADER_ADAPTER)
  target_compile_definitions(dummy-adapter
    INTERFACE
    LOADER_ADAPTER_HEADER="testing/dummy_value_adapter.h"
    LOADER_ADAPTER_NAME=${dummy_adapter_name}
    LOADER_ADAPTER_TYPE=flow::test::DummyValueAdapter
  )
endif ()

add_library(proxels SHARED
  dummy.cpp
  mummy.cpp
  yummy.cpp
  )

target_link_libraries(proxels
  PRIVATE 
    superflow::core
    superflow::dummy-adapter
  )

set_target_properties(proxels PROPERTIES
    CXX_STANDARD_REQUIRED ON
    CXX_STANDARD 17
    DEBUG_POSTFIX ""
    )

configure_file(
  "${CMAKE_CURRENT_LIST_DIR}/lib_path.h.in"
  "${CMAKE_CURRENT_LIST_DIR}/lib_path.h"
  )

file(GENERATE
  OUTPUT "${CMAKE_CURRENT_LIST_DIR}/lib_path.h"
  INPUT "${CMAKE_CURRENT_LIST_DIR}/lib_path.h"
  )

