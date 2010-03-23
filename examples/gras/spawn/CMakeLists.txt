cmake_minimum_required(VERSION 2.6)

set_source_files_properties(_spawn_simulator.c _spawn_child.c _spawn_father.c
PROPERTIES GENERATED true)

set(EXECUTABLE_OUTPUT_PATH "./")
set(LIBRARY_OUTPUT_PATH "${PROJECT_DIRECTORY}/lib")

add_executable(spawn_simulator _spawn_simulator.c spawn_father.c spawn_child.c spawn_common.c)
add_executable(spawn_child _spawn_child.c spawn_child.c spawn_common.c)
add_executable(spawn_father _spawn_father.c  spawn_father.c spawn_child.c spawn_common.c)

### Add definitions for compile
target_link_libraries(spawn_simulator simgrid pthread m -fprofile-arcs)
target_link_libraries(spawn_child gras pthread m -fprofile-arcs)
target_link_libraries(spawn_father gras pthread m -fprofile-arcs)

