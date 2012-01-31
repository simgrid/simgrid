cmake_minimum_required(VERSION 2.6)

set(EXECUTABLE_OUTPUT_PATH "${CMAKE_CURRENT_BINARY_DIR}")

add_executable(file file.c)

### Add definitions for compile
if(NOT WIN32)
target_link_libraries(file simgrid m pthread )
else(NOT WIN32)
target_link_libraries(file simgrid)
endif(NOT WIN32)

