cmake_minimum_required(VERSION 2.6)

set(EXECUTABLE_OUTPUT_PATH "${PROJECT_DIRECTORY}/examples/simdag/properties/")

add_executable(sd_prop sd_prop.c)

### Add definitions for compile
if(NOT WIN32)
target_link_libraries(sd_prop simgrid pthread m )
else(NOT WIN32)
target_link_libraries(sd_prop simgrid)
endif(NOT WIN32)
