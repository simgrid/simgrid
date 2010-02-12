cmake_minimum_required(VERSION 2.6)

set(EXECUTABLE_OUTPUT_PATH "./")
set(LIBRARY_OUTPUT_PATH "${PROJECT_DIRECTORY}/lib")

add_executable(saturate_simulator _saturate_simulator.c saturate.c)
add_executable(saturate_maestro _saturate_maestro.c saturate.c)
add_executable(saturate_sensor _saturate_sensor.c saturate.c)

### Add definitions for compile
target_link_libraries(saturate_simulator simgrid pthread m -fprofile-arcs)
target_link_libraries(saturate_maestro gras pthread m -fprofile-arcs)
target_link_libraries(saturate_sensor gras pthread m -fprofile-arcs)
