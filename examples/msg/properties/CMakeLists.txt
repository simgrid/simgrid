cmake_minimum_required(VERSION 2.6)

set(EXECUTABLE_OUTPUT_PATH "${CMAKE_CURRENT_BINARY_DIR}")

add_executable(msg_prop msg_prop.c)

### Add definitions for compile
target_link_libraries(msg_prop simgrid m )
