cmake_minimum_required(VERSION 2.6)

set_source_files_properties(${PROJECT_DIRECTORY}/examples/gras/ping/_ping_simulator.c ${PROJECT_DIRECTORY}/examples/gras/ping/_ping_client.c ${PROJECT_DIRECTORY}/examples/gras/ping/_ping_server.c
PROPERTIES GENERATED true)

set(EXECUTABLE_OUTPUT_PATH "${PROJECT_DIRECTORY}/examples/gras/ping/")

add_executable(ping_simulator ${PROJECT_DIRECTORY}/examples/gras/ping/_ping_simulator.c ping_server.c ${PROJECT_DIRECTORY}/examples/gras/ping/ping_client.c ${PROJECT_DIRECTORY}/examples/gras/ping/ping_common.c)
add_executable(ping_client ${PROJECT_DIRECTORY}/examples/gras/ping/_ping_client.c ${PROJECT_DIRECTORY}/examples/gras/ping/ping_client.c ping_common.c)
add_executable(ping_server ${PROJECT_DIRECTORY}/examples/gras/ping/_ping_server.c ${PROJECT_DIRECTORY}/examples/gras/ping/ping_server.c ping_common.c)

### Add definitions for compile
target_link_libraries(ping_simulator simgrid pthread m -fprofile-arcs)
target_link_libraries(ping_client gras pthread m -fprofile-arcs)
target_link_libraries(ping_server gras pthread m -fprofile-arcs)
