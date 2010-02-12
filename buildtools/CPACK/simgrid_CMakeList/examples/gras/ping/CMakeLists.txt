cmake_minimum_required(VERSION 2.6)

set(EXECUTABLE_OUTPUT_PATH "./")
set(LIBRARY_OUTPUT_PATH "${PROJECT_DIRECTORY}/lib")

add_executable(ping_simulator _ping_simulator.c ping_server.c ping_client.c ping_common.c)
add_executable(ping_client _ping_client.c ping_client.c ping_common.c)
add_executable(ping_server _ping_server.c ping_server.c ping_common.c)

### Add definitions for compile
target_link_libraries(ping_simulator simgrid pthread m -fprofile-arcs)
target_link_libraries(ping_client gras pthread m -fprofile-arcs)
target_link_libraries(ping_server gras pthread m -fprofile-arcs)
