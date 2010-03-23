cmake_minimum_required(VERSION 2.6)

set_source_files_properties(_synchro_simulator.c _synchro_philosopher.c
PROPERTIES GENERATED true)

set(EXECUTABLE_OUTPUT_PATH "./")
set(LIBRARY_OUTPUT_PATH "${PROJECT_DIRECTORY}/lib")

add_executable(synchro_simulator _synchro_simulator.c philosopher.c)
add_executable(synchro_philosopher _synchro_philosopher.c  philosopher.c)

### Add definitions for compile
target_link_libraries(synchro_simulator simgrid pthread m -fprofile-arcs)
target_link_libraries(synchro_philosopher gras pthread m -fprofile-arcs)
