cmake_minimum_required(VERSION 2.6)

set(EXECUTABLE_OUTPUT_PATH "./")
set(LIBRARY_OUTPUT_PATH "${PROJECT_DIRECTORY}/lib")
set(INCLUDES "-I${PROJECT_DIRECTORY}/include -I${PROJECT_DIRECTORY}/src/include -I${PROJECT_DIRECTORY}/src -I/usr/include")

set(CMAKE_CXX_FLAGS -fprofile-arcs -ftest-coverage)
add_definitions (${INCLUDES} -fprofile-arcs -ftest-coverage)

add_executable(timer_simulator _timer_simulator.c timer.c)
add_executable(timer_client _timer_client.c timer.c)

### Add definitions for compile
target_link_libraries(timer_simulator simgrid pthread m -fprofile-arcs)
target_link_libraries(timer_client gras pthread m -fprofile-arcs)
