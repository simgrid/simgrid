cmake_minimum_required(VERSION 2.6)

set(EXECUTABLE_OUTPUT_PATH "./")
set(LIBRARY_OUTPUT_PATH "${PROJECT_DIRECTORY}/lib")

add_executable(bandwidth_simulator _bandwidth_simulator.c bandwidth.c)
add_executable(bandwidth_maestro _bandwidth_maestro.c bandwidth.c)
add_executable(bandwidth_sensor _bandwidth_sensor.c bandwidth.c)

### Add definitions for compile
target_link_libraries(bandwidth_simulator simgrid pthread m -fprofile-arcs)
target_link_libraries(bandwidth_maestro gras pthread m -fprofile-arcs)
target_link_libraries(bandwidth_sensor gras pthread m -fprofile-arcs)
