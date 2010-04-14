cmake_minimum_required(VERSION 2.6)

set_source_files_properties(${PROJECT_DIRECTORY}/examples/amok/saturate/_saturate_simulator.c ${PROJECT_DIRECTORY}/examples/amok/saturate/_saturate_maestro.c ${PROJECT_DIRECTORY}/examples/amok/saturate/_saturate_sensor.c
PROPERTIES GENERATED true)

set(EXECUTABLE_OUTPUT_PATH "./")
set(LIBRARY_OUTPUT_PATH "${PROJECT_DIRECTORY}/lib")

add_executable(saturate_simulator ${PROJECT_DIRECTORY}/examples/amok/saturate/_saturate_simulator.c ${PROJECT_DIRECTORY}/examples/amok/saturate/saturate.c)
add_executable(saturate_maestro ${PROJECT_DIRECTORY}/examples/amok/saturate/_saturate_maestro.c ${PROJECT_DIRECTORY}/examples/amok/saturate/saturate.c)
add_executable(saturate_sensor ${PROJECT_DIRECTORY}/examples/amok/saturate/_saturate_sensor.c ${PROJECT_DIRECTORY}/examples/amok/saturate/saturate.c)

### Add definitions for compile
target_link_libraries(saturate_simulator simgrid pthread m -fprofile-arcs)
target_link_libraries(saturate_maestro gras pthread m -fprofile-arcs)
target_link_libraries(saturate_sensor gras pthread m -fprofile-arcs)
