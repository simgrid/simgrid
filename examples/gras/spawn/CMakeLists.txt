cmake_minimum_required(VERSION 2.6)

set_source_files_properties(${PROJECT_DIRECTORY}/examples/gras/spawn/_spawn_simulator.c ${PROJECT_DIRECTORY}/examples/gras/spawn/_spawn_child.c ${PROJECT_DIRECTORY}/examples/gras/spawn/_spawn_father.c
PROPERTIES GENERATED true)

set(EXECUTABLE_OUTPUT_PATH "./")
set(LIBRARY_OUTPUT_PATH "${PROJECT_DIRECTORY}/lib")

add_executable(spawn_simulator ${PROJECT_DIRECTORY}/examples/gras/spawn/_spawn_simulator.c ${PROJECT_DIRECTORY}/examples/gras/spawn/spawn_father.c ${PROJECT_DIRECTORY}/examples/gras/spawn/spawn_child.c ${PROJECT_DIRECTORY}/examples/gras/spawn/spawn_common.c)
add_executable(spawn_child ${PROJECT_DIRECTORY}/examples/gras/spawn/_spawn_child.c ${PROJECT_DIRECTORY}/examples/gras/spawn/spawn_child.c ${PROJECT_DIRECTORY}/examples/gras/spawn/spawn_common.c)
add_executable(spawn_father ${PROJECT_DIRECTORY}/examples/gras/spawn/_spawn_father.c  ${PROJECT_DIRECTORY}/examples/gras/spawn/spawn_father.c ${PROJECT_DIRECTORY}/examples/gras/spawn/spawn_child.c ${PROJECT_DIRECTORY}/examples/gras/spawn/spawn_common.c)

### Add definitions for compile
target_link_libraries(spawn_simulator simgrid pthread m -fprofile-arcs)
target_link_libraries(spawn_child gras pthread m -fprofile-arcs)
target_link_libraries(spawn_father gras pthread m -fprofile-arcs)

