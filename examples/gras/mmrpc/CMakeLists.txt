cmake_minimum_required(VERSION 2.6)

set_source_files_properties(${CMAKE_CURRENT_BINARY_DIR}/_mmrpc_simulator.c 
							${CMAKE_CURRENT_BINARY_DIR}/_mmrpc_client.c 
							${CMAKE_CURRENT_BINARY_DIR}/_mmrpc_server.c
							PROPERTIES GENERATED true)

set(EXECUTABLE_OUTPUT_PATH "${CMAKE_CURRENT_SOURCE_DIR}")

add_executable(mmrpc_simulator 	${CMAKE_CURRENT_BINARY_DIR}/_mmrpc_simulator.c 
								${CMAKE_CURRENT_SOURCE_DIR}/mmrpc_common.c
								${CMAKE_CURRENT_SOURCE_DIR}/mmrpc_client.c
								${CMAKE_CURRENT_SOURCE_DIR}/mmrpc_server.c)
add_executable(mmrpc_client ${CMAKE_CURRENT_BINARY_DIR}/_mmrpc_client.c
							${CMAKE_CURRENT_SOURCE_DIR}/mmrpc_common.c
							${CMAKE_CURRENT_SOURCE_DIR}/mmrpc_client.c)
add_executable(mmrpc_server ${CMAKE_CURRENT_BINARY_DIR}/_mmrpc_server.c
							${CMAKE_CURRENT_SOURCE_DIR}/mmrpc_common.c
							${CMAKE_CURRENT_SOURCE_DIR}/mmrpc_server.c)

add_custom_command(
  					OUTPUT 	${CMAKE_CURRENT_BINARY_DIR}/_mmrpc_simulator.c 
							${CMAKE_CURRENT_BINARY_DIR}/_mmrpc_client.c 
							${CMAKE_CURRENT_BINARY_DIR}/_mmrpc_server.c
  					DEPENDS gras_stub_generator ${CMAKE_CURRENT_SOURCE_DIR}/mmrpc.xml
  					COMMAND ${CMAKE_BINARY_DIR}/bin/gras_stub_generator mmrpc ${CMAKE_CURRENT_SOURCE_DIR}/mmrpc.xml
					)
					
### Add definitions for compile
target_link_libraries(mmrpc_simulator simgrid pthread m )
target_link_libraries(mmrpc_client gras pthread m )
target_link_libraries(mmrpc_server gras pthread m )
