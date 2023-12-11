set(PARENT_PROJECT ${PROJECT_NAME})
set(target_test_name "${CMAKE_PROJECT_NAME}-${PARENT_PROJECT}-test")
project(${target_test_name} CXX)
message(STATUS "* Adding test executable '${target_test_name}'")

add_executable(${target_test_name}
  "connectable_port.h"
  "connectable_proxel.h"
  "crashing_proxel.h"
  "multi_connectable_port.h"
  "pimpl_test.cpp"
  "templated_testproxel.h"
  "test_block_lock_queue.cpp"
  "test_buffered_consumer_port.cpp"
  "test_callback_consumer_port.cpp"
  "test_connection_manager.cpp"
  "test_graph_factory.cpp"
  "test_graph.cpp"
  "test_interface_port.cpp"
  "test_lock_queue.cpp"
  "test_multi_lock_queue.cpp"
  "test_pimpl.cpp"
  "test_requester_responder_port.cpp"
  "test_metronome.cpp"
  "test_multi_consumer_port.cpp"
  "test_multi_queue_getter.cpp"
  "test_multi_requester_port.cpp"
  "test_mutexed.cpp"
  "test_port_manager.cpp"
  "test_proxel_status.cpp"
  "test_producer_consumer_port.cpp"
  "test_proxel_timer.cpp"
  "test_shared_mutexed.cpp"
  "test_signal_waiter.cpp"
  "test_sleeper.cpp"
  "test_throttle.cpp"
  "threaded_proxel.h"
)

target_link_libraries(
  ${target_test_name}
  PRIVATE GTest::gtest GTest::gtest_main
  PRIVATE ${CMAKE_PROJECT_NAME}::core
)

set_target_properties(${target_test_name} PROPERTIES
  CXX_STANDARD_REQUIRED ON
  CXX_STANDARD 17
)

include(GoogleTest)
gtest_discover_tests(${target_test_name})
