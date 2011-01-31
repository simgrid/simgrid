cmake_minimum_required(VERSION 2.6)

set(EXECUTABLE_OUTPUT_PATH "${CMAKE_CURRENT_BINARY_DIR}")

add_executable(token_ring ring_call.c)

### Add definitions for compile
if(NOT WIN32)
target_link_libraries(token_ring simgrid m pthread )
else(NOT WIN32)
target_link_libraries(token_ring simgrid)
endif(NOT WIN32)

