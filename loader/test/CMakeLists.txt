set(PARENT_PROJECT ${PROJECT_NAME})
set(target_test_name "${CMAKE_PROJECT_NAME}-${PARENT_PROJECT}-test")
project(${target_test_name} CXX)
message(STATUS "* Adding test executable '${target_test_name}'")

add_subdirectory(proxels)


add_executable(${target_test_name}
  "test_load_factories.cpp"
  "test_proxel_library.cpp"
  "$<$<CXX_COMPILER_ID:GNU>:test_rtld_now.cpp>"
)

target_link_libraries(
  ${target_test_name}
  PRIVATE 
    GTest::gtest GTest::gtest_main
    ${CMAKE_PROJECT_NAME}::core
    ${CMAKE_PROJECT_NAME}::loader
    superflow::dummy-adapter
  )

set_target_properties(${target_test_name} PROPERTIES
  CXX_STANDARD_REQUIRED ON
  CXX_STANDARD 17
  )

file(GENERATE
  OUTPUT "$<TARGET_FILE_DIR:proxels>/libdependent_library.so"
  INPUT "${CMAKE_CURRENT_LIST_DIR}/libs/libdependent_library.so"
)

file(GENERATE
  OUTPUT "$<TARGET_FILE_DIR:proxels>/libmissing_dependency.so"
  INPUT "${CMAKE_CURRENT_LIST_DIR}/libs/libmissing_dependency.so"
)

include(GoogleTest)
gtest_discover_tests(${target_test_name})
