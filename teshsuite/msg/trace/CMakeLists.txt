cmake_minimum_required(VERSION 2.6)

set(EXECUTABLE_OUTPUT_PATH "${CMAKE_CURRENT_BINARY_DIR}")

add_executable(test_trace_integration test_trace_integration.c)

### Add definitions for compile
target_link_libraries(test_trace_integration simgrid m )
