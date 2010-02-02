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

add_custom_command(TARGET ex_sd_test
POST_BUILD
COMMAND "cp" 
ARGS ./ex_sd_test ./sd_test
WORKING_DIRECTORY "${PROJECT_DIRECTORY}/examples/simdag"
)
add_custom_command(TARGET ex_sd_test2
POST_BUILD
COMMAND "cp" 
ARGS ./ex_sd_test2 ./sd_test2
WORKING_DIRECTORY "${PROJECT_DIRECTORY}/examples/simdag"
)
