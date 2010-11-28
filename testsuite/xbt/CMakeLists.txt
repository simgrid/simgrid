cmake_minimum_required(VERSION 2.6)

set(EXECUTABLE_OUTPUT_PATH "${CMAKE_CURRENT_BINARY_DIR}/testsuite/xbt/")

add_executable(log_usage "log_usage.c")
add_executable(graphxml_usage "graphxml_usage.c")
add_executable(heap_bench "heap_bench.c")

### Add definitions for compile
if(NOT WIN32)
target_link_libraries(log_usage gras m )
target_link_libraries(graphxml_usage simgrid m )
target_link_libraries(heap_bench gras m )
else(NOT WIN32)
target_link_libraries(log_usage gras )
target_link_libraries(graphxml_usage simgrid )
target_link_libraries(heap_bench gras )
endif(NOT WIN32)