cmake_minimum_required(VERSION 2.6)

set(EXECUTABLE_OUTPUT_PATH "${CMAKE_CURRENT_BINARY_DIR}")

add_executable(chord "chord.c")
add_executable(chord_stateful "chord_stateful.c")

### Add definitions for compile
target_link_libraries(chord simgrid )
target_link_libraries(chord_stateful simgrid)
