cmake_minimum_required(VERSION 2.6)

set(EXECUTABLE_OUTPUT_PATH "${PROJECT_DIRECTORY}/teshsuite/simdag/partask/")

add_executable(test_comp_only_seq test_comp_only_seq.c)
add_executable(test_comp_only_par test_comp_only_par.c)

### Add definitions for compile
target_link_libraries(test_comp_only_seq simgrid m pthread )
target_link_libraries(test_comp_only_par simgrid m pthread )
