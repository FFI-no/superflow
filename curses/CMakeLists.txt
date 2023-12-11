project(curses)
init_module()

find_package(ncursescpp REQUIRED)

add_library_boilerplate()

target_link_libraries(${target_name}
  PUBLIC
    superflow::core
  PRIVATE
    ncursescpp::ncursescpp
  )

if (BUILD_TESTS)
  add_subdirectory(test)
endif()