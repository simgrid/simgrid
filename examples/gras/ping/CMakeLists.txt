cmake_minimum_required(VERSION 2.6)

set_source_files_properties(${CMAKE_CURRENT_BINARY_DIR}/_ping_simulator.c
							${CMAKE_CURRENT_BINARY_DIR}/_ping_client.c 
							${CMAKE_CURRENT_BINARY_DIR}/_ping_server.c
                            PROPERTIES GENERATED true)

set(EXECUTABLE_OUTPUT_PATH "${CMAKE_CURRENT_BINARY_DIR}")

add_executable(ping_simulator 	${CMAKE_CURRENT_BINARY_DIR}/_ping_simulator.c
								${CMAKE_CURRENT_SOURCE_DIR}/ping_server.c
								${CMAKE_CURRENT_SOURCE_DIR}/ping_client.c
								${CMAKE_CURRENT_SOURCE_DIR}/ping_common.c)
add_executable(ping_client    	${CMAKE_CURRENT_BINARY_DIR}/_ping_client.c
								${CMAKE_CURRENT_SOURCE_DIR}/ping_client.c
								${CMAKE_CURRENT_SOURCE_DIR}/ping_common.c)
add_executable(ping_server    	${CMAKE_CURRENT_BINARY_DIR}/_ping_server.c
								${CMAKE_CURRENT_SOURCE_DIR}/ping_server.c
								${CMAKE_CURRENT_SOURCE_DIR}/ping_common.c)
add_custom_command(
  					OUTPUT 	${CMAKE_CURRENT_BINARY_DIR}/_ping_simulator.c 
         					${CMAKE_CURRENT_BINARY_DIR}/_ping_client.c 
	 						${CMAKE_CURRENT_BINARY_DIR}/_ping_server.c
  					DEPENDS gras_stub_generator ${CMAKE_CURRENT_SOURCE_DIR}/ping.xml
  					COMMAND ${CMAKE_BINARY_DIR}/bin/gras_stub_generator ping ${CMAKE_CURRENT_SOURCE_DIR}/ping.xml
					)

### Add definitions for compile
if(NOT WIN32)
target_link_libraries(ping_simulator simgrid pthread m )
target_link_libraries(ping_client gras pthread m )
target_link_libraries(ping_server gras pthread m )
else(NOT WIN32)
target_link_libraries(ping_simulator simgrid)
target_link_libraries(ping_client gras)
target_link_libraries(ping_server gras)
endif(NOT WIN32)