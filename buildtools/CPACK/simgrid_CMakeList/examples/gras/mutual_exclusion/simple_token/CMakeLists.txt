cmake_minimum_required(VERSION 2.6)

set(EXECUTABLE_OUTPUT_PATH "./")
set(LIBRARY_OUTPUT_PATH "${PROJECT_DIRECTORY}/lib")

add_executable(simple_token_simulator _simple_token_simulator.c simple_token.c)
add_executable(simple_token_node _simple_token_node.c simple_token.c)

### Add definitions for compile
target_link_libraries(simple_token_simulator simgrid pthread m -fprofile-arcs)
target_link_libraries(simple_token_node gras pthread m -fprofile-arcs)

