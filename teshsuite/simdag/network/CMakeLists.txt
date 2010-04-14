cmake_minimum_required(VERSION 2.6)

set(EXECUTABLE_OUTPUT_PATH "${PROJECT_DIRECTORY}/teshsuite/simdag/network/")

add_executable(test_reinit_costs test_reinit_costs.c)


### Add definitions for compile
target_link_libraries(test_reinit_costs simgrid m pthread -fprofile-arcs)

