cmake_minimum_required(VERSION 2.6)

set(EXECUTABLE_OUTPUT_PATH "./")
set(LIBRARY_OUTPUT_PATH "${PROJECT_DIRECTORY}/lib")
set(INCLUDES "-I${PROJECT_DIRECTORY}/include -I${PROJECT_DIRECTORY}/src/include -I${PROJECT_DIRECTORY}/src -I/usr/include")

set(CMAKE_CXX_FLAGS -fprofile-arcs -ftest-coverage)
add_definitions (${INCLUDES} -fprofile-arcs -ftest-coverage)

add_executable(log_large_test log_large_test.c)
add_executable(parallel_log_crashtest parallel_log_crashtest.c)

### Add definitions for compile
target_link_libraries(log_large_test gras m pthread -fprofile-arcs)
target_link_libraries(parallel_log_crashtest gras m pthread -fprofile-arcs)
