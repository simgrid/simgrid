cmake_minimum_required(VERSION 2.6)

set(EXECUTABLE_OUTPUT_PATH "${PROJECT_DIRECTORY}/teshsuite/gras/datadesc/")

add_executable(datadesc_usage datadesc_usage.c datadesc_structs.c)

### Add definitions for compile
target_link_libraries(datadesc_usage gras m pthread)
