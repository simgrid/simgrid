cmake_minimum_required(VERSION 2.6)

set_source_files_properties(${CMAKE_CURRENT_BINARY_DIR}/_properties_simulator.c
							${CMAKE_CURRENT_BINARY_DIR}/_properties_alice.c
							${CMAKE_CURRENT_BINARY_DIR}/_properties_bob.c
							PROPERTIES GENERATED true)

set(EXECUTABLE_OUTPUT_PATH "${CMAKE_CURRENT_SOURCE_DIR}")

add_executable(properties_simulator ${CMAKE_CURRENT_BINARY_DIR}/_properties_simulator.c
									${CMAKE_CURRENT_SOURCE_DIR}/properties.c)
add_executable(properties_alice ${CMAKE_CURRENT_BINARY_DIR}/_properties_alice.c
								${CMAKE_CURRENT_SOURCE_DIR}/properties.c)
add_executable(properties_bob 	${CMAKE_CURRENT_BINARY_DIR}/_properties_bob.c
								${CMAKE_CURRENT_SOURCE_DIR}/properties.c)
add_custom_command(
  					OUTPUT 	${CMAKE_CURRENT_BINARY_DIR}/_properties_simulator.c
							${CMAKE_CURRENT_BINARY_DIR}/_properties_alice.c
							${CMAKE_CURRENT_BINARY_DIR}/_properties_bob.c
  					DEPENDS gras_stub_generator ${CMAKE_CURRENT_SOURCE_DIR}/properties.xml
  					COMMAND ${CMAKE_BINARY_DIR}/bin/gras_stub_generator properties ${CMAKE_CURRENT_SOURCE_DIR}/properties.xml
					)
					
### Add definitions for compile
if(NOT WIN32)
target_link_libraries(properties_simulator simgrid pthread m )
target_link_libraries(properties_alice gras pthread m )
target_link_libraries(properties_bob gras pthread m )
else(NOT WIN32)
target_link_libraries(properties_simulator simgrid)
target_link_libraries(properties_alice gras)
target_link_libraries(properties_bob gras)
endif(NOT WIN32)

