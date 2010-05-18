cmake_minimum_required(VERSION 2.6)

set_source_files_properties(${CMAKE_CURRENT_BINARY_DIR}/_simple_token_simulator.c
							${CMAKE_CURRENT_BINARY_DIR}/_simple_token_node.c
							PROPERTIES GENERATED true)

set(EXECUTABLE_OUTPUT_PATH "${CMAKE_CURRENT_SOURCE_DIR}")

add_executable(simple_token_simulator 	${CMAKE_CURRENT_BINARY_DIR}/_simple_token_simulator.c
										${CMAKE_CURRENT_SOURCE_DIR}/simple_token.c)
add_executable(simple_token_node 	${CMAKE_CURRENT_BINARY_DIR}/_simple_token_node.c
									${CMAKE_CURRENT_SOURCE_DIR}/simple_token.c)
									
add_custom_command(
  					OUTPUT 	${CMAKE_CURRENT_BINARY_DIR}/_simple_token_simulator.c 
         					${CMAKE_CURRENT_BINARY_DIR}/_simple_token_node.c 
  					DEPENDS gras_stub_generator ${CMAKE_CURRENT_SOURCE_DIR}/simple_token.xml
  					COMMAND ${CMAKE_BINARY_DIR}/bin/gras_stub_generator simple_token ${CMAKE_CURRENT_SOURCE_DIR}/simple_token.xml
					)

### Add definitions for compile
target_link_libraries(simple_token_simulator simgrid pthread m -fprofile-arcs)
target_link_libraries(simple_token_node gras pthread m -fprofile-arcs)

