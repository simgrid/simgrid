cmake_minimum_required(VERSION 2.6)

set(EXECUTABLE_OUTPUT_PATH "./")
set(LIBRARY_OUTPUT_PATH "${PROJECT_DIRECTORY}/lib")
set(INCLUDES "-I${PROJECT_DIRECTORY}/include -I${PROJECT_DIRECTORY}/src/include -I${PROJECT_DIRECTORY}/src -I/usr/include")

set(CMAKE_CXX_FLAGS -fprofile-arcs -ftest-coverage)
add_definitions (${INCLUDES} -fprofile-arcs -ftest-coverage)

add_executable(log_usage "log_usage.c")
add_executable(graphxml_usage "graphxml_usage.c")
add_executable(heap_bench "heap_bench.c")

### Add definitions for compile
target_link_libraries(log_usage gras m -fprofile-arcs)
target_link_libraries(graphxml_usage simgrid m -fprofile-arcs)
target_link_libraries(heap_bench gras m -fprofile-arcs)
