cmake_minimum_required(VERSION 2.6)

set(EXECUTABLE_OUTPUT_PATH "${PROJECT_DIRECTORY}/examples/simdag/scheduling/")

add_executable(minmin_test minmin_test.c)

### Add definitions for compile
target_link_libraries(minmin_test simgrid pthread m)
