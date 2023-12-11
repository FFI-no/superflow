project(loader)
init_module()

find_package(Boost REQUIRED system filesystem)

add_library_boilerplate()

target_link_libraries(${target_name}
  PUBLIC
  Boost::headers
  Boost::filesystem
  Boost::system
  superflow::core
  INTERFACE
  ${CMAKE_DL_LIBS}
  )

if (MSVC)
  target_link_libraries(${target_name} INTERFACE Boost::dynamic_linking)
endif()

if (BUILD_TESTS)
  add_subdirectory(test)
endif()
