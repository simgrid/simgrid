cmake_minimum_required(VERSION 2.6)

set(EXECUTABLE_OUTPUT_PATH "./")
set(LIBRARY_OUTPUT_PATH "${PROJECT_DIRECTORY}/lib")
set(INCLUDES "-I${PROJECT_DIRECTORY}/include -I${PROJECT_DIRECTORY}/src/include -I${PROJECT_DIRECTORY}/src -I/usr/include")

set(CMAKE_CXX_FLAGS -fprofile-arcs -ftest-coverage)
add_definitions (${INCLUDES} -fprofile-arcs -ftest-coverage)

add_executable(properties_simulator _properties_simulator.c properties.c)
add_executable(properties_alice _properties_alice.c    properties.c)
add_executable(properties_bob _properties_bob.c    properties.c)

### Add definitions for compile
target_link_libraries(properties_simulator simgrid pthread m -fprofile-arcs)
target_link_libraries(properties_alice gras pthread m -fprofile-arcs)
target_link_libraries(properties_bob gras pthread m -fprofile-arcs)
