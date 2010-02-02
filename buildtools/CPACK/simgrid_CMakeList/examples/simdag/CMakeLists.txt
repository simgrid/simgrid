cmake_minimum_required(VERSION 2.6)

set(EXECUTABLE_OUTPUT_PATH "./")
set(LIBRARY_OUTPUT_PATH "${PROJECT_DIRECTORY}/lib")
set(INCLUDES "-I${PROJECT_DIRECTORY}/include -I${PROJECT_DIRECTORY}/src/include -I${PROJECT_DIRECTORY}/src -I/usr/include")

set(CMAKE_CXX_FLAGS -fprofile-arcs -ftest-coverage)
add_definitions (${INCLUDES} -fprofile-arcs -ftest-coverage)

add_executable(ex_sd_test sd_test.c)
add_executable(ex_sd_test2 sd_test2.c)

### Add definitions for compile
target_link_libraries(ex_sd_test simgrid pthread m -fprofile-arcs)
target_link_libraries(ex_sd_test2 simgrid pthread m -fprofile-arcs)
