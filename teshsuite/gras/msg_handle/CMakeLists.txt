cmake_minimum_required(VERSION 2.6)

set_source_files_properties(${PROJECT_DIRECTORY}/teshsuite/gras/msg_handle/_msg_handle_simulator.c ${PROJECT_DIRECTORY}/teshsuite/gras/msg_handle/_msg_handle_client.c ${PROJECT_DIRECTORY}/teshsuite/gras/msg_handle/_msg_handle_server.c
PROPERTIES GENERATED true)

set(EXECUTABLE_OUTPUT_PATH "./")
set(LIBRARY_OUTPUT_PATH "${PROJECT_DIRECTORY}/lib")

add_executable(msg_handle_simulator ${PROJECT_DIRECTORY}/teshsuite/gras/msg_handle/msg_handle.c ${PROJECT_DIRECTORY}/teshsuite/gras/msg_handle/_msg_handle_simulator.c)
add_executable(msg_handle_client ${PROJECT_DIRECTORY}/teshsuite/gras/msg_handle/msg_handle.c ${PROJECT_DIRECTORY}/teshsuite/gras/msg_handle/_msg_handle_client.c)
add_executable(msg_handle_server ${PROJECT_DIRECTORY}/teshsuite/gras/msg_handle/msg_handle.c ${PROJECT_DIRECTORY}/teshsuite/gras/msg_handle/_msg_handle_server.c)

### Add definitions for compile
target_link_libraries(msg_handle_simulator simgrid m pthread -fprofile-arcs)
target_link_libraries(msg_handle_client gras m pthread -fprofile-arcs)
target_link_libraries(msg_handle_server gras m pthread -fprofile-arcs)
