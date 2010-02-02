cmake_minimum_required(VERSION 2.6)

set(EXECUTABLE_OUTPUT_PATH "./")
set(LIBRARY_OUTPUT_PATH "${PROJECT_DIRECTORY}/lib")
set(INCLUDES "-I${PROJECT_DIRECTORY}/include -I${PROJECT_DIRECTORY}/src/include -I${PROJECT_DIRECTORY}/src -I/usr/include")

set(CMAKE_CXX_FLAGS -fprofile-arcs -ftest-coverage)
add_definitions (${INCLUDES} -fprofile-arcs -ftest-coverage)

add_executable(mmrpc_simulator _mmrpc_simulator.c mmrpc_common.c mmrpc_client.c mmrpc_server.c)
add_executable(mmrpc_client _mmrpc_client.c    mmrpc_common.c mmrpc_client.c)
add_executable(mmrpc_server _mmrpc_server.c    mmrpc_common.c mmrpc_server.c)

### Add definitions for compile
target_link_libraries(mmrpc_simulator simgrid pthread m -fprofile-arcs)
target_link_libraries(mmrpc_client gras pthread m -fprofile-arcs)
target_link_libraries(mmrpc_server gras pthread m -fprofile-arcs)
