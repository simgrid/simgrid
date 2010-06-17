cmake_minimum_required(VERSION 2.6)

set_source_files_properties(${CMAKE_CURRENT_BINARY_DIR}/_msg_handle_simulator.c
							${CMAKE_CURRENT_BINARY_DIR}/_msg_handle_client.c
							${CMAKE_CURRENT_BINARY_DIR}/_msg_handle_server.c
							PROPERTIES GENERATED true)

set(EXECUTABLE_OUTPUT_PATH "${CMAKE_CURRENT_SOURCE_DIR}")

add_executable(msg_handle_simulator ${CMAKE_CURRENT_SOURCE_DIR}/msg_handle.c
									${CMAKE_CURRENT_BINARY_DIR}/_msg_handle_simulator.c)
add_executable(msg_handle_client 	${CMAKE_CURRENT_SOURCE_DIR}/msg_handle.c
									${CMAKE_CURRENT_BINARY_DIR}/_msg_handle_client.c)
add_executable(msg_handle_server 	${CMAKE_CURRENT_SOURCE_DIR}/msg_handle.c
									${CMAKE_CURRENT_BINARY_DIR}/_msg_handle_server.c)

add_custom_command(
  					OUTPUT 	${CMAKE_CURRENT_BINARY_DIR}/_msg_handle_simulator.c
							${CMAKE_CURRENT_BINARY_DIR}/_msg_handle_client.c
							${CMAKE_CURRENT_BINARY_DIR}/_msg_handle_server.c
  					DEPENDS gras_stub_generator ${CMAKE_CURRENT_SOURCE_DIR}/msg_handle.xml
  					COMMAND ${CMAKE_BINARY_DIR}/bin/gras_stub_generator msg_handle ${CMAKE_CURRENT_SOURCE_DIR}/msg_handle.xml
					)

### Add definitions for compile
target_link_libraries(msg_handle_simulator simgrid m pthread )
target_link_libraries(msg_handle_client gras m pthread )
target_link_libraries(msg_handle_server gras m pthread )
