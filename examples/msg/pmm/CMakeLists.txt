cmake_minimum_required(VERSION 2.6)

set(EXECUTABLE_OUTPUT_PATH "${CMAKE_CURRENT_BINARY_DIR}")

add_executable(msg_pmm     msg_pmm.c)

target_link_libraries(msg_pmm simgrid m )
