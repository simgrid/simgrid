cmake_minimum_required(VERSION 2.6)

set(EXECUTABLE_OUTPUT_PATH "${CMAKE_CURRENT_BINARY_DIR}")

add_executable(log_usage "log_usage.c")
add_executable(graphxml_usage "graphxml_usage.c")
add_executable(heap_bench "heap_bench.c")
add_executable(parmap_bench "parmap_bench.c")

### Add definitions for compile
if(NOT WIN32)
target_link_libraries(log_usage gras m )
target_link_libraries(graphxml_usage simgrid m )
target_link_libraries(heap_bench gras m )
target_link_libraries(parmap_bench simgrid m )
else(NOT WIN32)
target_link_libraries(log_usage gras )
target_link_libraries(graphxml_usage simgrid )
target_link_libraries(heap_bench gras )
target_link_libraries(parmap_bench simgrid )
endif(NOT WIN32)
