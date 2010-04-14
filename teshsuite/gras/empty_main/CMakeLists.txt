cmake_minimum_required(VERSION 2.6)

set_source_files_properties(${PROJECT_DIRECTORY}/teshsuite/gras/empty_main/_empty_main_function.c ${PROJECT_DIRECTORY}/teshsuite/gras/empty_main/_empty_main_simulator.c
PROPERTIES GENERATED true)

set(EXECUTABLE_OUTPUT_PATH "${PROJECT_DIRECTORY}/teshsuite/gras/empty_main/")

add_executable(empty_main_function 	${PROJECT_DIRECTORY}/teshsuite/gras/empty_main/empty_main.c ${PROJECT_DIRECTORY}/teshsuite/gras/empty_main/_empty_main_function.c)
add_executable(empty_main_simulator 	${PROJECT_DIRECTORY}/teshsuite/gras/empty_main/empty_main.c ${PROJECT_DIRECTORY}/teshsuite/gras/empty_main/_empty_main_simulator.c)

### Add definitions for compile
target_link_libraries(empty_main_simulator simgrid m pthread -fprofile-arcs)
target_link_libraries(empty_main_function gras m pthread -fprofile-arcs)
