cmake_minimum_required(VERSION 2.6)

set_source_files_properties(${PROJECT_DIRECTORY}/examples/gras/pmm/_pmm_simulator.c ${PROJECT_DIRECTORY}/examples/gras/pmm/_pmm_slave.c ${PROJECT_DIRECTORY}/examples/gras/pmm/_pmm_master.c
PROPERTIES GENERATED true)

set(EXECUTABLE_OUTPUT_PATH "${PROJECT_DIRECTORY}/examples/gras/pmm/")

add_executable(pmm_simulator ${PROJECT_DIRECTORY}/examples/gras/pmm/_pmm_simulator.c ${PROJECT_DIRECTORY}/examples/gras/pmm/pmm.c)
add_executable(pmm_slave ${PROJECT_DIRECTORY}/examples/gras/pmm/_pmm_slave.c ${PROJECT_DIRECTORY}/examples/gras/pmm/pmm.c)
add_executable(pmm_master ${PROJECT_DIRECTORY}/examples/gras/pmm/_pmm_master.c ${PROJECT_DIRECTORY}/examples/gras/pmm/pmm.c)

### Add definitions for compile
target_link_libraries(pmm_simulator simgrid pthread m -fprofile-arcs)
target_link_libraries(pmm_slave gras pthread m -fprofile-arcs)
target_link_libraries(pmm_master gras pthread m -fprofile-arcs)
