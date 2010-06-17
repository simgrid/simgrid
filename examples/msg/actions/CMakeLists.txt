cmake_minimum_required(VERSION 2.6)

set(EXECUTABLE_OUTPUT_PATH "${PROJECT_DIRECTORY}/examples/msg/actions/")

add_executable(actions actions.c)

### Add definitions for compile
target_link_libraries(actions simgrid m )
