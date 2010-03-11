cmake_minimum_required(VERSION 2.6)

set(EXECUTABLE_OUTPUT_PATH "./")
set(LIBRARY_OUTPUT_PATH "${PROJECT_DIRECTORY}/lib")

add_executable(suspend suspend.c)

### Add definitions for compile
target_link_libraries(suspend simgrid m -fprofile-arcs)
