cmake_minimum_required(VERSION 2.6)

set(EXECUTABLE_OUTPUT_PATH "${PROJECT_DIRECTORY}/teshsuite/simdag/network/mxn/")

add_executable(test_intra_all2all test_intra_all2all.c)
add_executable(test_intra_independent_comm test_intra_independent_comm.c)
add_executable(test_intra_scatter test_intra_scatter.c)

### Add definitions for compile
target_link_libraries(test_intra_all2all simgrid m pthread -fprofile-arcs)
target_link_libraries(test_intra_independent_comm simgrid m pthread -fprofile-arcs)
target_link_libraries(test_intra_scatter simgrid m pthread -fprofile-arcs)
