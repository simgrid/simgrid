cmake_minimum_required(VERSION 2.6)

set(EXECUTABLE_OUTPUT_PATH "./")
set(LIBRARY_OUTPUT_PATH "${PROJECT_DIRECTORY}/lib")
set(INCLUDES "-I${PROJECT_DIRECTORY}/include -I${PROJECT_DIRECTORY}/src/include -I${PROJECT_DIRECTORY}/src -I/usr/include")

set(CMAKE_CXX_FLAGS -fprofile-arcs -ftest-coverage)
add_definitions (${INCLUDES} -fprofile-arcs -ftest-coverage)

add_executable(msg_handle_simulator msg_handle.c _msg_handle_simulator.c)
add_executable(msg_handle_client msg_handle.c _msg_handle_client.c)
add_executable(msg_handle_server msg_handle.c _msg_handle_server.c)

### Add definitions for compile
target_link_libraries(msg_handle_simulator simgrid m pthread -fprofile-arcs)
target_link_libraries(msg_handle_client gras m pthread -fprofile-arcs)
target_link_libraries(msg_handle_server gras m pthread -fprofile-arcs)
