cmake_minimum_required(VERSION 2.6)

set(EXECUTABLE_OUTPUT_PATH "${CMAKE_CURRENT_BINARY_DIR}")

add_executable(pcontexts "pcontexts.c")

### Add definitions for compile
if(WIN32)
target_link_libraries(pcontexts simgrid )
else(WIN32)
target_link_libraries(pcontexts simgrid m )
endif(WIN32)
