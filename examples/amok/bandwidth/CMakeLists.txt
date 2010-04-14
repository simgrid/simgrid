cmake_minimum_required(VERSION 2.6)

set_source_files_properties(${PROJECT_DIRECTORY}/examples/amok/bandwidth/_bandwidth_simulator.c ${PROJECT_DIRECTORY}/examples/amok/bandwidth/_bandwidth_maestro.c ${PROJECT_DIRECTORY}/examples/amok/bandwidth/_bandwidth_sensor.c
PROPERTIES GENERATED true)

set(EXECUTABLE_OUTPUT_PATH "${PROJECT_DIRECTORY}/examples/amok/bandwidth/")

add_executable(bandwidth_simulator ${PROJECT_DIRECTORY}/examples/amok/bandwidth/_bandwidth_simulator.c ${PROJECT_DIRECTORY}/examples/amok/bandwidth/bandwidth.c)
add_executable(bandwidth_maestro ${PROJECT_DIRECTORY}/examples/amok/bandwidth/_bandwidth_maestro.c ${PROJECT_DIRECTORY}/examples/amok/bandwidth/bandwidth.c)
add_executable(bandwidth_sensor ${PROJECT_DIRECTORY}/examples/amok/bandwidth/_bandwidth_sensor.c ${PROJECT_DIRECTORY}/examples/amok/bandwidth/bandwidth.c)

### Add definitions for compile
target_link_libraries(bandwidth_simulator simgrid pthread m -fprofile-arcs)
target_link_libraries(bandwidth_maestro gras pthread m -fprofile-arcs)
target_link_libraries(bandwidth_sensor gras pthread m -fprofile-arcs)
