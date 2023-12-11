project(core)
init_module()

find_package(Threads REQUIRED)

add_library_boilerplate()

target_link_libraries(${target_name}
  PUBLIC
    Threads::Threads
)

if (BUILD_TESTS)
  add_subdirectory(test)
endif ()
