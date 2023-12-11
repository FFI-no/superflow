set(PARENT_PROJECT ${PROJECT_NAME})
project(${CMAKE_PROJECT_NAME}-${PARENT_PROJECT}-test CXX)

message(STATUS "* Adding test executable '${PROJECT_NAME}'")
add_executable(${PROJECT_NAME}
  "yaml-test-proxel.cpp"
  "test_yaml_property_list.cpp"
)

target_link_libraries(
  ${PROJECT_NAME}
  PRIVATE
  GTest::gtest GTest::gtest_main
  ${CMAKE_PROJECT_NAME}::loader
  ${CMAKE_PROJECT_NAME}::yaml
)

set_target_properties(${PROJECT_NAME} PROPERTIES
  CXX_STANDARD_REQUIRED ON
  CXX_STANDARD 17
  ENABLE_EXPORTS ON
)

include(GoogleTest)
gtest_discover_tests(${PROJECT_NAME})
