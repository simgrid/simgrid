cmake_minimum_required(VERSION 2.6)

set_source_files_properties(${PROJECT_DIRECTORY}/examples/gras/timer/_timer_simulator.c ${PROJECT_DIRECTORY}/examples/gras/timer/_timer_client.c
PROPERTIES GENERATED true)

set(EXECUTABLE_OUTPUT_PATH "./")
set(LIBRARY_OUTPUT_PATH "${PROJECT_DIRECTORY}/lib")

add_executable(timer_simulator ${PROJECT_DIRECTORY}/examples/gras/timer/_timer_simulator.c ${PROJECT_DIRECTORY}/examples/gras/timer/timer.c)
add_executable(timer_client ${PROJECT_DIRECTORY}/examples/gras/timer/_timer_client.c ${PROJECT_DIRECTORY}/examples/gras/timer/timer.c)

### Add definitions for compile
target_link_libraries(timer_simulator simgrid pthread m -fprofile-arcs)
target_link_libraries(timer_client gras pthread m -fprofile-arcs)
