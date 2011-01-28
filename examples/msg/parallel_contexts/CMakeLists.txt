cmake_minimum_required(VERSION 2.6)

set(EXECUTABLE_OUTPUT_PATH "${CMAKE_CURRENT_BINARY_DIR}")

add_executable(pcontexts "pcontexts.c")
add_executable(pcontexts2 "pcontexts2.c")

### Add definitions for compile
if(WIN32)
target_link_libraries(pcontexts simgrid )
target_link_libraries(pcontexts2 simgrid )
else(WIN32)
target_link_libraries(pcontexts simgrid m )
target_link_libraries(pcontexts2 simgrid m )
endif(WIN32)
