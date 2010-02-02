cmake_minimum_required(VERSION 2.6)

set(EXECUTABLE_OUTPUT_PATH "./")
set(LIBRARY_OUTPUT_PATH "${PROJECT_DIRECTORY}/lib")
set(INCLUDES "-I${PROJECT_DIRECTORY}/include -I${PROJECT_DIRECTORY}/src/include -I${PROJECT_DIRECTORY}/src -I/usr/include")

set(CMAKE_CXX_FLAGS -fprofile-arcs -ftest-coverage)
add_definitions (${INCLUDES} -fprofile-arcs -ftest-coverage)

add_executable(rpc_simulator _rpc_simulator.c rpc.c)
add_executable(rpc_client _rpc_client.c rpc.c)
add_executable(rpc_forwarder _rpc_forwarder.c rpc.c)
add_executable(rpc_server _rpc_server.c rpc.c)

### Add definitions for compile
target_link_libraries(rpc_simulator simgrid pthread m -fprofile-arcs)
target_link_libraries(rpc_client gras pthread m -fprofile-arcs)
target_link_libraries(rpc_forwarder gras pthread m -fprofile-arcs)
target_link_libraries(rpc_server gras pthread m -fprofile-arcs)
