cmake_minimum_required(VERSION 2.6)

set_source_files_properties(${PROJECT_DIRECTORY}/examples/gras/rpc/_rpc_simulator.c ${PROJECT_DIRECTORY}/examples/gras/rpc/_rpc_client.c ${PROJECT_DIRECTORY}/examples/gras/rpc/_rpc_forwarder.c ${PROJECT_DIRECTORY}/examples/gras/rpc/_rpc_server.c
PROPERTIES GENERATED true)

set(EXECUTABLE_OUTPUT_PATH "./")
set(LIBRARY_OUTPUT_PATH "${PROJECT_DIRECTORY}/lib")

add_executable(rpc_simulator ${PROJECT_DIRECTORY}/examples/gras/rpc/_rpc_simulator.c ${PROJECT_DIRECTORY}/examples/gras/rpc/rpc.c)
add_executable(rpc_client ${PROJECT_DIRECTORY}/examples/gras/rpc/_rpc_client.c ${PROJECT_DIRECTORY}/examples/gras/rpc/rpc.c)
add_executable(rpc_forwarder ${PROJECT_DIRECTORY}/examples/gras/rpc/_rpc_forwarder.c ${PROJECT_DIRECTORY}/examples/gras/rpc/rpc.c)
add_executable(rpc_server ${PROJECT_DIRECTORY}/examples/gras/rpc/_rpc_server.c ${PROJECT_DIRECTORY}/examples/gras/rpc/rpc.c)

### Add definitions for compile
target_link_libraries(rpc_simulator simgrid pthread m -fprofile-arcs)
target_link_libraries(rpc_client gras pthread m -fprofile-arcs)
target_link_libraries(rpc_forwarder gras pthread m -fprofile-arcs)
target_link_libraries(rpc_server gras pthread m -fprofile-arcs)
