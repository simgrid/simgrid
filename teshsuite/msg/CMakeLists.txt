cmake_minimum_required(VERSION 2.6)

set(EXECUTABLE_OUTPUT_PATH "${PROJECT_DIRECTORY}/teshsuite/msg/")	

add_executable(get_sender ${PROJECT_DIRECTORY}/teshsuite/msg/get_sender.c)

### Add definitions for compile
target_link_libraries(get_sender simgrid m pthread )
