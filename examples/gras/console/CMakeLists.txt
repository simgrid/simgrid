if(HAVE_LUA)
cmake_minimum_required(VERSION 2.6)

set_source_files_properties(${CMAKE_CURRENT_BINARY_DIR}/_ping_simulator.c
							${CMAKE_CURRENT_BINARY_DIR}/_ping_client.c 
							${CMAKE_CURRENT_BINARY_DIR}/_ping_server.c
                            PROPERTIES GENERATED true)

set(EXECUTABLE_OUTPUT_PATH "${CMAKE_CURRENT_SOURCE_DIR}")

add_executable(ping_simulator_console 	${CMAKE_CURRENT_BINARY_DIR}/_ping_simulator.c
								${CMAKE_CURRENT_SOURCE_DIR}/ping_server.c
								${CMAKE_CURRENT_SOURCE_DIR}/ping_client.c
								${CMAKE_CURRENT_SOURCE_DIR}/ping_common.c)
add_executable(ping_client_console    	${CMAKE_CURRENT_BINARY_DIR}/_ping_client.c
								${CMAKE_CURRENT_SOURCE_DIR}/ping_client.c
								${CMAKE_CURRENT_SOURCE_DIR}/ping_common.c)
add_executable(ping_server_console    	${CMAKE_CURRENT_BINARY_DIR}/_ping_server.c
								${CMAKE_CURRENT_SOURCE_DIR}/ping_server.c
								${CMAKE_CURRENT_SOURCE_DIR}/ping_common.c)

add_custom_command(
  					OUTPUT 	${CMAKE_CURRENT_BINARY_DIR}/_ping_simulator.c 
         					${CMAKE_CURRENT_BINARY_DIR}/_ping_client.c 
	 						${CMAKE_CURRENT_BINARY_DIR}/_ping_server.c
  					COMMAND lua ${CMAKE_CURRENT_SOURCE_DIR}/ping_generator.lua
					)
### Add definitions for compile
target_link_libraries(ping_simulator_console simgrid pthread m )
target_link_libraries(ping_client_console gras pthread m )
target_link_libraries(ping_server_console gras pthread m )
endif(HAVE_LUA)
