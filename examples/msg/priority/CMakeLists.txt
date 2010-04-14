cmake_minimum_required(VERSION 2.6)

set(EXECUTABLE_OUTPUT_PATH "${PROJECT_DIRECTORY}/examples/msg/priority/")

add_executable(priority priority.c)

### Add definitions for compile
target_link_libraries(priority simgrid m -fprofile-arcs)
