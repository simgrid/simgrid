cmake_minimum_required(VERSION 2.6)

set(EXECUTABLE_OUTPUT_PATH "./")
set(LIBRARY_OUTPUT_PATH "${PROJECT_DIRECTORY}/lib")
set(INCLUDES "-I${PROJECT_DIRECTORY}/include -I${PROJECT_DIRECTORY}/src/include -I${PROJECT_DIRECTORY}/src -I/usr/include")

set(CMAKE_CXX_FLAGS -fprofile-arcs -ftest-coverage)
add_definitions (${INCLUDES} -fprofile-arcs -ftest-coverage)

add_executable(test_comp_only_seq test_comp_only_seq.c)
add_executable(test_comp_only_par test_comp_only_par.c)

### Add definitions for compile
target_link_libraries(test_comp_only_seq simgrid m pthread -fprofile-arcs)
target_link_libraries(test_comp_only_par simgrid m pthread -fprofile-arcs)
