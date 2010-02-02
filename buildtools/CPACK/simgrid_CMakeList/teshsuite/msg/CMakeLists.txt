cmake_minimum_required(VERSION 2.6)

set(EXECUTABLE_OUTPUT_PATH "./")
set(LIBRARY_OUTPUT_PATH "${PROJECT_DIRECTORY}/lib")
set(INCLUDES "-I${PROJECT_DIRECTORY}/include -I${PROJECT_DIRECTORY}/src/include -I${PROJECT_DIRECTORY}/src -I/usr/include")

set(CMAKE_CXX_FLAGS -fprofile-arcs -ftest-coverage)
add_definitions (${INCLUDES} -fprofile-arcs -ftest-coverage)

add_executable(get_sender get_sender.c)

### Add definitions for compile
target_link_libraries(get_sender simgrid m pthread -fprofile-arcs)
