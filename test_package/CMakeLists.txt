cmake_minimum_required(VERSION 3.15)
project(PackageTest CXX)

set(name superflow)
list(APPEND CMAKE_MODULE_PATH ${CMAKE_CURRENT_BINARY_DIR})
list(APPEND CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR})

enable_testing()
find_package(${name} REQUIRED core curses loader yaml)

include(GenerateTestFromModule)

set(modules core curses loader yaml)
foreach(module ${modules})
  generate_test_from_module(${name} ${module})
endforeach()

if(TARGET superflow::loader)
  target_link_libraries(superflow_loader_runtime_test superflow::yaml)
endif()
