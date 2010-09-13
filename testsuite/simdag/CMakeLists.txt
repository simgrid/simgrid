cmake_minimum_required(VERSION 2.6)

set(EXECUTABLE_OUTPUT_PATH "${PROJECT_DIRECTORY}/testsuite/simdag/")

add_executable(sd_test sd_test.c)

### Add definitions for compile
if(NOT WIN32)
target_link_libraries(sd_test simgrid m ) 
else(NOT WIN32)
target_link_libraries(sd_test simgrid ) 
endif(NOT WIN32)
