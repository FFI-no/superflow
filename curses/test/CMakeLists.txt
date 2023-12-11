set(PARENT_PROJECT ${PROJECT_NAME})
set(target_test_name "${CMAKE_PROJECT_NAME}-${PARENT_PROJECT}-test")
project(${target_test_name} CXX)
message(STATUS "* Adding test executable '${target_test_name}'")

add_executable(${target_test_name}
  "test_superflow_curses.cpp"
)

target_link_libraries(
  ${target_test_name}
  PRIVATE GTest::gtest GTest::gtest_main
  PRIVATE ${CMAKE_PROJECT_NAME}::curses
)

set_target_properties(${target_test_name} PROPERTIES
  CXX_STANDARD_REQUIRED ON
  CXX_STANDARD 17
)

include(GoogleTest)
gtest_discover_tests(${target_test_name})
