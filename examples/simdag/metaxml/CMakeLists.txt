cmake_minimum_required(VERSION 2.6)

set(EXECUTABLE_OUTPUT_PATH "${CMAKE_CURRENT_BINARY_DIR}")

add_executable(sd_meta sd_meta.c)

### Add definitions for compile
if(NOT WIN32)
target_link_libraries(sd_meta simgrid pthread m )
else(NOT WIN32)
target_link_libraries(sd_meta simgrid)
endif(NOT WIN32)