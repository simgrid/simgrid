cmake_minimum_required(VERSION 2.6)

set(EXECUTABLE_OUTPUT_PATH "${PROJECT_DIRECTORY}/examples/simdag/dax/")

add_executable(dax_test dax_test.c)

### Add definitions for compile
target_link_libraries(dax_test simgrid pthread m -fprofile-arcs)
