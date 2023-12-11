project(yaml)
init_module()

find_package(yaml-cpp REQUIRED)

add_library_boilerplate()

option(SET_LOADER_ADAPTER "Set the LOADER_ADAPTER_xxx macros. Turn off if you are linking more than one adapter." ON)
if (SET_LOADER_ADAPTER)
  target_compile_definitions(${target_name}
    INTERFACE
    LOADER_ADAPTER_HEADER="superflow/yaml/yaml_property_list.h"
    LOADER_ADAPTER_NAME=YAML
    LOADER_ADAPTER_TYPE=flow::yaml::YAMLPropertyList
  )
endif ()

target_link_libraries(${target_name}
  PUBLIC
    superflow::core
    yaml-cpp::yaml-cpp
  )

if (BUILD_TESTS)
  add_subdirectory(test)
endif()
