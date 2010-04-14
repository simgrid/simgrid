cmake_minimum_required(VERSION 2.6)

set_source_files_properties(${PROJECT_DIRECTORY}/examples/gras/synchro/_synchro_simulator.c ${PROJECT_DIRECTORY}/examples/gras/synchro/_synchro_philosopher.c
PROPERTIES GENERATED true)

set(EXECUTABLE_OUTPUT_PATH "./")
set(LIBRARY_OUTPUT_PATH "${PROJECT_DIRECTORY}/lib")

add_executable(synchro_simulator ${PROJECT_DIRECTORY}/examples/gras/synchro/_synchro_simulator.c ${PROJECT_DIRECTORY}/examples/gras/synchro/philosopher.c)
add_executable(synchro_philosopher ${PROJECT_DIRECTORY}/examples/gras/synchro/_synchro_philosopher.c  ${PROJECT_DIRECTORY}/examples/gras/synchro/philosopher.c)

### Add definitions for compile
target_link_libraries(synchro_simulator simgrid pthread m -fprofile-arcs)
target_link_libraries(synchro_philosopher gras pthread m -fprofile-arcs)
