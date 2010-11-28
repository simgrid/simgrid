cmake_minimum_required(VERSION 2.6)

set(EXECUTABLE_OUTPUT_PATH "${CMAKE_CURRENT_BINARY_DIR}")

add_executable(dax_test dax_test.c)

### Add definitions for compile
if(NOT WIN32)
target_link_libraries(dax_test simgrid pthread m )
else(NOT WIN32)
target_link_libraries(dax_test simgrid)
endif(NOT WIN32)
