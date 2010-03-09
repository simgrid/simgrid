cmake_minimum_required(VERSION 2.6)

set_source_files_properties(_empty_main_function.c _empty_main_simulator.c
PROPERTIES GENERATED true)

set(EXECUTABLE_OUTPUT_PATH "./")
set(LIBRARY_OUTPUT_PATH "${PROJECT_DIRECTORY}/lib")

add_executable(empty_main_function 	empty_main.c _empty_main_function.c)
add_executable(empty_main_simulator 	empty_main.c _empty_main_simulator.c)

### Add definitions for compile
target_link_libraries(empty_main_simulator simgrid m pthread -fprofile-arcs)
target_link_libraries(empty_main_function gras m pthread -fprofile-arcs)
