cmake_minimum_required(VERSION 2.6)

set_source_files_properties(${PROJECT_DIRECTORY}/examples/gras/mmrpc/_mmrpc_simulator.c ${PROJECT_DIRECTORY}/examples/gras/mmrpc/_mmrpc_client.c ${PROJECT_DIRECTORY}/examples/gras/mmrpc/_mmrpc_server.c
PROPERTIES GENERATED true)

set(EXECUTABLE_OUTPUT_PATH "./")
set(LIBRARY_OUTPUT_PATH "${PROJECT_DIRECTORY}/lib")

add_executable(mmrpc_simulator ${PROJECT_DIRECTORY}/examples/gras/mmrpc/_mmrpc_simulator.c ${PROJECT_DIRECTORY}/examples/gras/mmrpc/mmrpc_common.c ${PROJECT_DIRECTORY}/examples/gras/mmrpc/mmrpc_client.c ${PROJECT_DIRECTORY}/examples/gras/mmrpc/mmrpc_server.c)
add_executable(mmrpc_client ${PROJECT_DIRECTORY}/examples/gras/mmrpc/_mmrpc_client.c    ${PROJECT_DIRECTORY}/examples/gras/mmrpc/mmrpc_common.c ${PROJECT_DIRECTORY}/examples/gras/mmrpc/mmrpc_client.c)
add_executable(mmrpc_server ${PROJECT_DIRECTORY}/examples/gras/mmrpc/_mmrpc_server.c    ${PROJECT_DIRECTORY}/examples/gras/mmrpc/mmrpc_common.c ${PROJECT_DIRECTORY}/examples/gras/mmrpc/mmrpc_server.c)

### Add definitions for compile
target_link_libraries(mmrpc_simulator simgrid pthread m -fprofile-arcs)
target_link_libraries(mmrpc_client gras pthread m -fprofile-arcs)
target_link_libraries(mmrpc_server gras pthread m -fprofile-arcs)
