cmake_minimum_required(VERSION 2.6)

set(EXECUTABLE_OUTPUT_PATH "./")
set(LIBRARY_OUTPUT_PATH "${PROJECT_DIRECTORY}/lib")

add_executable(small_sleep_function 	small_sleep.c _small_sleep_function.c)
add_executable(small_sleep_simulator 	small_sleep.c _small_sleep_simulator.c)

### Add definitions for compile
target_link_libraries(small_sleep_simulator simgrid m pthread -fprofile-arcs)
target_link_libraries(small_sleep_function gras m pthread -fprofile-arcs)
