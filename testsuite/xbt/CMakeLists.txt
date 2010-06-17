cmake_minimum_required(VERSION 2.6)

set(EXECUTABLE_OUTPUT_PATH "${PROJECT_DIRECTORY}/testsuite/xbt/")

add_executable(log_usage "log_usage.c")
add_executable(graphxml_usage "graphxml_usage.c")
add_executable(heap_bench "heap_bench.c")

### Add definitions for compile
target_link_libraries(log_usage gras m )
target_link_libraries(graphxml_usage simgrid m )
target_link_libraries(heap_bench gras m )
