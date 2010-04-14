cmake_minimum_required(VERSION 2.6)

set_source_files_properties(${PROJECT_DIRECTORY}/examples/gras/mutual_exclusion/simple_token/_simple_token_simulator.c ${PROJECT_DIRECTORY}/examples/gras/mutual_exclusion/simple_token/_simple_token_node.c
PROPERTIES GENERATED true)

set(EXECUTABLE_OUTPUT_PATH "./")
set(LIBRARY_OUTPUT_PATH "${PROJECT_DIRECTORY}/lib")

add_executable(simple_token_simulator ${PROJECT_DIRECTORY}/examples/gras/mutual_exclusion/simple_token/_simple_token_simulator.c ${PROJECT_DIRECTORY}/examples/gras/mutual_exclusion/simple_token/simple_token.c)
add_executable(simple_token_node ${PROJECT_DIRECTORY}/examples/gras/mutual_exclusion/simple_token/_simple_token_node.c ${PROJECT_DIRECTORY}/examples/gras/mutual_exclusion/simple_token/simple_token.c)

### Add definitions for compile
target_link_libraries(simple_token_simulator simgrid pthread m -fprofile-arcs)
target_link_libraries(simple_token_node gras pthread m -fprofile-arcs)

