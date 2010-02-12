cmake_minimum_required(VERSION 2.6)

set(EXECUTABLE_OUTPUT_PATH "./")
set(LIBRARY_OUTPUT_PATH "${PROJECT_DIRECTORY}/lib")

add_executable(pmm_simulator _pmm_simulator.c pmm.c)
add_executable(pmm_slave _pmm_slave.c pmm.c)
add_executable(pmm_master _pmm_master.c pmm.c)

### Add definitions for compile
target_link_libraries(pmm_simulator simgrid pthread m -fprofile-arcs)
target_link_libraries(pmm_slave gras pthread m -fprofile-arcs)
target_link_libraries(pmm_master gras pthread m -fprofile-arcs)
