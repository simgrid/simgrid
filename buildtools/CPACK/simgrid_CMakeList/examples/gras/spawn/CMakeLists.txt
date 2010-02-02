cmake_minimum_required(VERSION 2.6)

set(EXECUTABLE_OUTPUT_PATH "./")
set(LIBRARY_OUTPUT_PATH "${PROJECT_DIRECTORY}/lib")
set(INCLUDES "-I${PROJECT_DIRECTORY}/include -I${PROJECT_DIRECTORY}/src/include -I${PROJECT_DIRECTORY}/src -I/usr/include")

set(CMAKE_CXX_FLAGS -fprofile-arcs -ftest-coverage)
add_definitions (${INCLUDES} -fprofile-arcs -ftest-coverage)

add_executable(spawn_simulator _spawn_simulator.c spawn_father.c spawn_child.c spawn_common.c)
add_executable(spawn_child _spawn_child.c spawn_child.c spawn_common.c)
add_executable(spawn_father _spawn_father.c  spawn_father.c spawn_child.c spawn_common.c)

### Add definitions for compile
target_link_libraries(spawn_simulator simgrid pthread m -fprofile-arcs)
target_link_libraries(spawn_child gras pthread m -fprofile-arcs)
target_link_libraries(spawn_father gras pthread m -fprofile-arcs)

