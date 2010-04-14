cmake_minimum_required(VERSION 2.6)

set_source_files_properties(${PROJECT_DIRECTORY}/teshsuite/gras/small_sleep/_small_sleep_function.c ${PROJECT_DIRECTORY}/teshsuite/gras/small_sleep/_small_sleep_simulator.c
PROPERTIES GENERATED true)

set(EXECUTABLE_OUTPUT_PATH "./")
set(LIBRARY_OUTPUT_PATH "${PROJECT_DIRECTORY}/lib")

add_executable(small_sleep_function 	${PROJECT_DIRECTORY}/teshsuite/gras/small_sleep/small_sleep.c ${PROJECT_DIRECTORY}/teshsuite/gras/small_sleep/_small_sleep_function.c)
add_executable(small_sleep_simulator 	${PROJECT_DIRECTORY}/teshsuite/gras/small_sleep/small_sleep.c ${PROJECT_DIRECTORY}/teshsuite/gras/small_sleep/_small_sleep_simulator.c)

### Add definitions for compile
target_link_libraries(small_sleep_simulator simgrid m pthread -fprofile-arcs)
target_link_libraries(small_sleep_function gras m pthread -fprofile-arcs)
