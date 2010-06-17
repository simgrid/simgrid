cmake_minimum_required(VERSION 2.6)

set(EXECUTABLE_OUTPUT_PATH "${PROJECT_DIRECTORY}/examples/msg/gtnets/")

add_executable(gtnets "gtnets.c")

### Add definitions for compile
target_link_libraries(gtnets simgrid m )

