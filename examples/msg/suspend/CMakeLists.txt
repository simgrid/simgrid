cmake_minimum_required(VERSION 2.6)

set(EXECUTABLE_OUTPUT_PATH "${PROJECT_DIRECTORY}/examples/msg/suspend/")

add_executable(suspend suspend.c)

### Add definitions for compile
target_link_libraries(suspend simgrid m )
