cmake_minimum_required(VERSION 2.6)

set_source_files_properties(${CMAKE_CURRENT_BINARY_DIR}/_saturate_simulator.c
							${CMAKE_CURRENT_BINARY_DIR}/_saturate_maestro.c
							${CMAKE_CURRENT_BINARY_DIR}/_saturate_sensor.c
							PROPERTIES GENERATED true)
							
set(EXECUTABLE_OUTPUT_PATH "${CMAKE_CURRENT_SOURCE_DIR}")

add_executable(saturate_simulator 	${CMAKE_CURRENT_BINARY_DIR}/_saturate_simulator.c
									${CMAKE_CURRENT_SOURCE_DIR}/saturate.c)
add_executable(saturate_maestro ${CMAKE_CURRENT_BINARY_DIR}/_saturate_maestro.c
								${CMAKE_CURRENT_SOURCE_DIR}/saturate.c)
add_executable(saturate_sensor 	${CMAKE_CURRENT_BINARY_DIR}/_saturate_sensor.c
								${CMAKE_CURRENT_SOURCE_DIR}/saturate.c)

add_custom_command(
  					OUTPUT 	${CMAKE_CURRENT_BINARY_DIR}/_saturate_simulator.c
							${CMAKE_CURRENT_BINARY_DIR}/_saturate_maestro.c
							${CMAKE_CURRENT_BINARY_DIR}/_saturate_sensor.c
  					DEPENDS gras_stub_generator ${CMAKE_CURRENT_SOURCE_DIR}/saturate.xml
  					COMMAND ${CMAKE_BINARY_DIR}/bin/gras_stub_generator saturate ${CMAKE_CURRENT_SOURCE_DIR}/saturate.xml
					)
					
### Add definitions for compile
if(NOT WIN32)
target_link_libraries(saturate_simulator simgrid pthread m )
target_link_libraries(saturate_maestro gras pthread m )
target_link_libraries(saturate_sensor gras pthread m )
else(NOT WIN32)
target_link_libraries(saturate_simulator simgrid)
target_link_libraries(saturate_maestro gras)
target_link_libraries(saturate_sensor gras)
endif(NOT WIN32)