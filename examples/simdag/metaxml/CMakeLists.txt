cmake_minimum_required(VERSION 2.6)

set(EXECUTABLE_OUTPUT_PATH "${PROJECT_DIRECTORY}/examples/simdag/metaxml/")

add_executable(sd_meta sd_meta.c)

### Add definitions for compile
target_link_libraries(sd_meta simgrid pthread m )
