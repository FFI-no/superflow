macro(generate_test_from_module namespace module_name)
  if(TARGET ${namespace}::${module_name})
    message(STATUS "generate_test_from_module(${namespace} ${module_name})")
    add_executable(${namespace}_${module_name}_runtime_test src/${namespace}${module_name}_runtime_test.cpp)
    target_link_libraries(${namespace}_${module_name}_runtime_test ${namespace}::${module_name})
    set_target_properties(${namespace}_${module_name}_runtime_test PROPERTIES
      CXX_STANDARD_REQUIRED ON
      CXX_STANDARD 17
      BUILD_RPATH $ORIGIN
      INSTALL_RPATH $ORIGIN
      )
    add_test(${namespace}_${module_name}_runtime_test ${namespace}_${module_name}_runtime_test)
  else()
    message(WARNING "generate_test_from_module: nonexisting target ${namespace}::${module_name}")
  endif()
endmacro()
