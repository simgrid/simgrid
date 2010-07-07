cmake_minimum_required(VERSION 2.6)

set(EXECUTABLE_OUTPUT_PATH "${PROJECT_DIRECTORY}/examples/msg/icomms/")

add_executable(peer peer.c)
target_link_libraries(peer simgrid)

add_executable(peer2 peer2.c)
target_link_libraries(peer2 simgrid)