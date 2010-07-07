cmake_minimum_required(VERSION 2.6)

set(EXECUTABLE_OUTPUT_PATH "${PROJECT_DIRECTORY}/examples/msg/irc_isend/")

add_executable(peer peer.c)
target_link_libraries(peer simgrid)